#include <sycl/sycl.hpp>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <format>

#include <libcvrp/sycl/local-search.hpp>

#include <libcvrp/core/constants.hpp>
#include <libcvrp/sycl/splitter.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp>
#include <libcvrp/sycl/Top3Insertion.hpp>
#include <libcvrp/sycl/RoutePair.hpp>

namespace cvrp::sycl_engine::local_search
{
void 
optimize(std::vector<int>& mega_tour, Instance& instance, int start_id, ExecutionContext& ctx)
{
  auto routes = import_mega_tour(mega_tour, instance);

  apply_two_opt(mega_tour, routes, instance);

  int* my_device_tour = ctx.d_mega_tour;
  DeviceRoute* my_device_routes = ctx.d_routes;
  Top3Insertion* my_device_top3_matrix = ctx.d_top3_matrix;
  BestSwap* my_best_swap = ctx.d_best_swap;

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes, instance, ContextData{my_device_tour, my_device_routes, my_device_top3_matrix, my_best_swap}, ctx);

  ctx.q.memcpy(mega_tour.data(), my_device_tour, mega_tour.size() * sizeof(int)).wait();
}

std::vector<DeviceRoute> 
import_mega_tour(std::vector<int> &mega_tour, Instance& instance)
{
  auto routes = naive_split(mega_tour, instance);

  auto& clients = instance.clients;

  for (auto& route : routes){
    
    route.sector.initialize(clients[mega_tour[route.start_index]].polarAngle);

    for (int i = 1; i < route.size; ++i)
      route.sector.extend(clients[mega_tour[route.start_index + i]].polarAngle);
  }

  return routes;
}

void 
apply_two_opt(std::vector<int>& mega_tour, std::vector<DeviceRoute>& routes, cvrp::Instance& instance)
{
  for(auto& route : routes){
    
    // Se a rota possuir menos de 2 clientes, um 2-opt não é possível
    if (route.size < 2) continue;

    bool improved = true;

    while (improved)
    {
      improved = false;
      
      for(int i = 0; i < route.size - 1; ++i)
      {
        for (int j = i + 1; j < route.size; ++j)
        {
          // O vizinho anterior a 'i'. Se 'i' for o primeiro cliente, o vizinho é o depósito
          int node_prev_i = (i == 0) ? 0 : mega_tour[route.start_index + i - 1];
          
          // Os clientes nos pontos de corte
          int node_i = mega_tour[route.start_index + i];
          int node_j = mega_tour[route.start_index + j];
          
          // O vizinho posterior a 'j'. Se 'j' for o último cliente, o vizinho é o depósito
          int node_next_j = (j == route.size - 1) ? 0 : mega_tour[route.start_index + j + 1];

          double old_dist = instance.client_distance(node_prev_i, node_i) + 
                            instance.client_distance(node_j, node_next_j);

          double new_dist = instance.client_distance(node_prev_i, node_j) + 
                            instance.client_distance(node_i, node_next_j);
          
          // Se a nova distância for menor, aplica a troca invertendo os elementos centrais
          if (new_dist < old_dist) {

            std::reverse(mega_tour.begin() + route.start_index + i, 
                         mega_tour.begin() + route.start_index + j + 1);

            improved = true;
          }
        }
      }
    }
  }
}

// Namespace anônimo para as funções auxiliares do swap star
namespace
{

  inline float 
  insertion_cost(int v, int p, int s, cvrp::sycl_engine::DeviceInstance instance) 
  {

    return instance.client_distance(p, v) +
           instance.client_distance(v, s) -
           instance.client_distance(p, s);
  }

  inline Top3Insertion
  findTop3Locations(int client_v, int* my_device_tour, DeviceRoute* r_ln, cvrp::sycl_engine::DeviceInstance instance)
  {   
    Top3Insertion top3;

    // Percorre os espaços entre os clientes para definir melhores inserções
    for (auto i = 0; i <= r_ln->size; ++i){

      // O vizinho anterior a 'i'. Se 'i' for o primeiro cliente, o vizinho é o depósito
      int node_prev_i = (i == 0) ? 0 : my_device_tour[r_ln->start_index + i - 1];

      // O vizinho posterior a 'i'. Se 'i' for o último cliente, o vizinho é o depósito
      int node_next_i = (i == r_ln->size) ? 0 : my_device_tour[r_ln->start_index + i ];

      top3.compareAndAdd(insert_info{i + r_ln->start_index, node_prev_i, node_next_i, insertion_cost(client_v, node_prev_i, node_next_i, instance)});
    }

    return top3;
  }

  inline Top3Insertion
  findTop3Locations(int client_v, int* rln_tile, DeviceRoute rln, cvrp::sycl_engine::DeviceInstance instance)
  {   
    Top3Insertion top3 = Top3Insertion::make();

    // Percorre os espaços entre os clientes para definir melhores inserções
    for (auto i = 0; i <= rln.size; ++i){

      // O vizinho anterior a 'i'. Se 'i' for o primeiro cliente, o vizinho é o depósito
      int node_prev = (i == 0) ? 0 : rln_tile[i - 1];

      // O vizinho posterior a 'i'. Se 'i' for o último cliente, o vizinho é o depósito
      int node_next = (i == rln.size) ? 0 : rln_tile[i];

      top3.compareAndAdd(insert_info{i + rln.start_index, node_prev, node_next, insertion_cost(client_v, node_prev, node_next, instance)});
    }

    return top3;
  }

  inline uint64_t 
  make_key(float cost, uint32_t id) 
  {
    uint32_t u = sycl::bit_cast<uint32_t>(cost);
    u = (u & 0x80000000u) ? ~u : (u | 0x80000000u);   // ordena floats como inteiros
    return (uint64_t(u) << 32) | id;
  }
  constexpr uint64_t NO_KEY = ~0ull;

  inline bool
  eval_pair_swap(u_int32_t t, 
                 int* r_tile, int* rln_tile, 
                 DeviceRoute r, DeviceRoute rln,
                 Top3Insertion* top3_v, Top3Insertion* top3_u,
                 DeviceInstance d_instance,
                 BestSwap& out_best)
  {

    int id_v = t / rln.size;
    int id_u = t % rln.size;

    int client_v = r_tile[id_v];
    int client_u = rln_tile[id_u];

    if (r.total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
      return false;
    
    if (rln.total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
      return false;

    // Melhor inserção de v em R', desconsiderando u da rota R'
    auto k = top3_v[id_v].get_best_insertion_except_client(client_u);

    // Melhor inserção de u em R, desconsiderando v da rota R
    auto kln = top3_u[id_u].get_best_insertion_except_client(client_v);

    int client_v_pred = id_v == 0 ? 0 : r_tile[id_v - 1];
    int client_v_suce = id_v == (r.size - 1) ? 0 : r_tile[id_v + 1];

    int client_u_pred = id_u == 0 ? 0 : rln_tile[id_u - 1];
    int client_u_suce = id_u == (rln.size - 1) ? 0 : rln_tile[id_u + 1];

    // Custo de inserção de V na exata posição de U
    float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);

    // Melhor custo de inserção de V em R'
    float v_to_rln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

    // Custo de inserção de U na exata posição de V
    float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
    
    // Melhor custo de inserção de U em r
    float u_to_r = sycl::fmin(swap_u_in_v, kln.cost)- insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

    out_best.total_cost = v_to_rln + u_to_r;

    out_best.v_tour_id = r.start_index + id_v;
    out_best.u_tour_id = rln.start_index + id_u;

    out_best.v_tour_dest = swap_v_in_u < k.cost ? rln.start_index + id_u : k.insert_index;
    out_best.u_tour_dest = swap_u_in_v < kln.cost ? r.start_index + id_v : kln.insert_index;


    return true;
  }
  
  /// @brief Realiza a troca de clientes entre duas rotas
  /// @param id_v Id do cliente a ser trocado na rota R, relativo a ela
  /// @param id_u Id do cliente a ser trocado na rota R', relativo a ela
  /// @param r Rota R participante da troca
  /// @param r_ln Rota R' participante da troca
  /// @param v_insert Informações de destino de inserção do cliente v na rota R'
  /// @param u_insert Informações de destino de inserção do cliente u na rota R
  /// @param instance Instância sendo avaliada
  void
  complete_swap_star(BestSwap* best_swap, 
                     int* mega_tour, 
                     DeviceRoute* r, 
                     DeviceRoute* r_ln, 
                     DeviceInstance instance)
  {
    auto clients = instance.clients;
    
    int client_v = mega_tour[best_swap->v_tour_id];
    int client_u = mega_tour[best_swap->u_tour_id];

    r->total_demand = r->total_demand - clients[client_v].demand + clients[client_u].demand;

    r_ln->total_demand = r_ln->total_demand - clients[client_u].demand + clients[client_v].demand;

    
    // Remove U da posição u_tour_id e insere V na posição v_tour_dest, atualizando a rota R'
    
    if (best_swap->v_tour_dest < best_swap->u_tour_id) {
        // Inserção antes da remoção: desloca elementos para a direita
      for (int i = best_swap->u_tour_id; i > best_swap->v_tour_dest; --i) {
        mega_tour[i] = mega_tour[i-1];
      }
      mega_tour[best_swap->v_tour_dest] = client_v;
    } 
    else if (best_swap->v_tour_dest > best_swap->u_tour_id) {
      // Inserção depois da remoção: desloca elementos para a esquerda. 
      // O destino real recua 1 posição devido à lacuna deixada por U.
      for (int i = best_swap->u_tour_id; i < best_swap->v_tour_dest - 1; ++i) {
        mega_tour[i] = mega_tour[i+1];
      }
      mega_tour[best_swap->v_tour_dest - 1] = client_v;
    } 
    else {
      // Substituição exata no mesmo índice
      mega_tour[best_swap->u_tour_id] = client_v;
    }

    // Remove V da posição v_tour_id e insere U na posição u_tour_dest, atualizando a rota R
    
    if (best_swap->u_tour_dest < best_swap->v_tour_id) {
      // Inserção antes da remoção: desloca elementos para a direita
      for (int i = best_swap->v_tour_id; i > best_swap->u_tour_dest; --i) {
        mega_tour[i] = mega_tour[i-1];
      }
      mega_tour[best_swap->u_tour_dest] = client_u;
    } 
    else if (best_swap->u_tour_dest > best_swap->v_tour_id) {
      // Inserção depois da remoção: desloca elementos para a esquerda. 
      // O destino real recua 1 posição devido à lacuna deixada por V.
      for (int i = best_swap->v_tour_id; i < best_swap->u_tour_dest - 1; ++i) {
        mega_tour[i] = mega_tour[i+1];
      }
      mega_tour[best_swap->u_tour_dest - 1] = client_u;
    } 
    else {
      // Substituição exata no mesmo índice
      mega_tour[best_swap->v_tour_id] = client_u;
    }
  }
}

void 
apply_swap_star(std::vector<DeviceRoute>& routes, 
                Instance& instance,
                ContextData my_ctx_data,
                ExecutionContext& ctx)
{
  auto& clients = instance.clients;

  auto [my_device_tour, my_device_routes, my_device_top3_matrix, my_best_swap] = my_ctx_data;

  cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;

  cvrp::sycl_engine::RoutePair* d_route_pairs = ctx.d_route_pairs;

  cvrp::sycl_engine::BestSwap* d_best_swap = ctx.d_best_swap;

  unsigned int instance_dimension = d_instance.dimension;

  int matrix_line_size = instance_dimension * ctx.max_routes;

  int total_route_combs = (routes.size() * (routes.size() - 1)) / 2;

  BestSwap identity {cvrp::INF_F, -1, -1, -1, -1, -1, -1};

  size_t block_size = 32;
  size_t global_size = total_route_combs * block_size;

  size_t max_r_size = std::max_element(routes.begin(), routes.end(), [](auto& a, auto& b) {
    return a.size < b.size;
  })->size;

  size_t local_bytes = max_r_size * (2 * sizeof(int) + 2 * sizeof(Top3Insertion));
  auto   local_lim   = ctx.q.get_device().get_info<sycl::info::device::local_mem_size>();

  if (local_bytes > local_lim)             throw std::runtime_error("memoria local insuficiente");
  if (routes.size() > (size_t)ctx.max_routes) throw std::runtime_error("rotas > max_routes");

  int max_routes_combs = ctx.max_routes * (ctx.max_routes - 1 ) / 2;
  if (total_route_combs > max_routes_combs) throw std::runtime_error("pares > max_route_combs");

  auto nd_main = ctx.q.submit([&] (sycl::handler& h){

    sycl::range<1> global{global_size};
    sycl::range<1> local{block_size};

    auto tile_r = sycl::local_accessor<int, 1>(max_r_size, h);
    auto tile_rln = sycl::local_accessor<int, 1>(max_r_size, h);

    auto top3_v = sycl::local_accessor<Top3Insertion, 1>(max_r_size, h);
    auto top3_u = sycl::local_accessor<Top3Insertion, 1>(max_r_size, h);

    h.parallel_for(sycl::nd_range<1>{global, local},
    [=](sycl::nd_item<1> it){

      auto gp = it.get_group();
      
      auto global_id = it.get_global_id();
      auto group_id = gp.get_group_linear_id();
      auto local_id = it.get_local_id(0);

      int r_i = d_route_pairs[group_id].first;
      int r_j = d_route_pairs[group_id].second;

      // Verificar se rotas se sobrepõem

      BestSwap* best_swap_r_rln = d_best_swap + group_id;

      DeviceRoute r = my_device_routes[r_i];
      DeviceRoute rln = my_device_routes[r_j];

      if (!r.sector.overlap(rln.sector)) {
        if (gp.leader()) 
          *best_swap_r_rln = identity;
        return;
      }

      auto r_ptr = tile_r.get_multi_ptr<sycl::access::decorated::no>().get();
      auto rln_ptr = tile_rln.get_multi_ptr<sycl::access::decorated::no>().get();
      
      // Carrega o conteúdo de R para a memória local
      for (auto k = local_id; k < r.size; k += block_size)
        tile_r[k] = my_device_tour[r.start_index + k];
      
      // Carrega o conteúdo de R' para a memória local
      for (auto k = local_id; k < rln.size; k += block_size)
        tile_rln[k] = my_device_tour[rln.start_index + k];

      sycl::group_barrier(gp);

      for (auto k = local_id; k < r.size + rln.size; k += block_size){

        if (k < r.size){

          // Calcular top3 de v em R'
          auto client_v = tile_r[k];

          top3_v[k] = findTop3Locations(client_v, rln_ptr, rln, d_instance);
        }

        else{
          auto real_k = k - r.size;

          // Calcular top3 de u em R
          auto client_u = tile_rln[real_k];
          top3_u[real_k] = findTop3Locations(client_u, r_ptr, r, d_instance);
        }
      }

      sycl::group_barrier(gp);

      auto top3_v_ptr = top3_v.get_multi_ptr<sycl::access::decorated::no>().get();
      auto top3_u_ptr = top3_u.get_multi_ptr<sycl::access::decorated::no>().get();

      uint64_t best = NO_KEY;

      // Acesso stride para que cada thread ache seu melhor par de trocas
      for (auto t = local_id; t < r.size * rln.size; t += block_size){
        BestSwap tmp;

        if (eval_pair_swap(t, r_ptr, rln_ptr, r, rln, top3_v_ptr, top3_u_ptr, d_instance, tmp)){
          best = sycl::min(best, make_key(tmp.total_cost, t));
        }
      }

      // Redução no grupo para encontrar o melhor par de troca de clientes entre as rotas R e R'

      uint64_t winner = sycl::reduce_over_group(gp, best, sycl::minimum<uint64_t>());

      if (gp.leader()){

        if (winner != NO_KEY){
          BestSwap result = identity;
          result.r_i = r_i; 
          result.r_j = r_j;
  
          eval_pair_swap((u_int32_t) winner, r_ptr, rln_ptr, r, rln, top3_v_ptr, top3_u_ptr, d_instance, result);
  
          *best_swap_r_rln = result;
        }
        else {
          *best_swap_r_rln = identity;
        }

      }
      
    });
  });
  ctx.q.wait_and_throw();

  ctx.q.submit([&](sycl::handler& h){

    h.depends_on(nd_main);

    sycl::range<1> global{1 * block_size};
    sycl::range<1> local{block_size};

    h.parallel_for(sycl::nd_range<1>{global, local},
    [=](sycl::nd_item<1> it){
      
      auto gp = it.get_group();
      auto local_id = it.get_local_id(0);

      u_int64_t k = NO_KEY;

      for (int c = local_id; c < total_route_combs; c += block_size){
        float cost = d_best_swap[c].total_cost;

        k = sycl::min(k, make_key(cost, c));
      }

      uint64_t winner = sycl::reduce_over_group(gp, k, sycl::minimum<uint64_t>());

      if (gp.leader() && winner != NO_KEY){

        BestSwap best = d_best_swap[uint32_t(winner)];

        if (best.total_cost < 0)
          complete_swap_star(&best, my_device_tour, my_device_routes + best.r_i, my_device_routes + best.r_j, d_instance);
      }

    });
    
  }).wait_and_throw();

}
} // namespace cvrp::sycl