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
optimize(std::vector<std::vector<int>*>& batch_tours, Instance& instance, ExecutionContext& ctx)
{
  
  std::vector<std::vector<DeviceRoute>> batch_routes;

  std::vector<int> batch_num_pairs;

  batch_routes.resize(batch_tours.size());
  batch_num_pairs.resize(batch_tours.size());

  // Extrai as rotas do batch de tours e aplica o 2-opt nelas
  for (int i = 0; i < batch_routes.size(); ++i){

    batch_routes[i] = import_mega_tour(*(batch_tours[i]), instance);

    batch_num_pairs[i] = batch_routes[i].size() * (batch_routes[i].size() - 1) / 2;

    apply_two_opt(*(batch_tours[i]), batch_routes[i], instance);
  }

  // Cópia dos mega_tours, rotas e seus tamanhos para a GPU
  for (int i = 0; i < batch_routes.size(); ++i){

    ctx.q.memcpy(ctx.d_mega_tour + i * ctx.peak_clients_per_tour, batch_tours[i]->data(), batch_tours[i]->size() * sizeof(int));
    ctx.q.memcpy(ctx.d_routes + i * ctx.peak_routes, batch_routes[i].data(), batch_routes[i].size() * sizeof(DeviceRoute));
  }
  
  ctx.q.memcpy(ctx.d_num_pairs, batch_num_pairs.data(), batch_num_pairs.size() * sizeof(int));

  ctx.q.wait();

  apply_swap_star(batch_routes, instance, ctx);

  // Cópia dos mega_tours, rotas e seus tamanhos da GPU para a CPU
  for (int i = 0; i < batch_routes.size(); ++i){

    ctx.q.memcpy(batch_tours[i]->data(), ctx.d_mega_tour + i * ctx.peak_clients_per_tour, batch_tours[i]->size() * sizeof(int));
    
  }

  ctx.q.wait();
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
  findTop3Locations(int client_v, int* d_tour, DeviceRoute* r_ln, cvrp::sycl_engine::DeviceInstance instance)
  {   
    Top3Insertion top3;

    // Percorre os espaços entre os clientes para definir melhores inserções
    for (auto i = 0; i <= r_ln->size; ++i){

      // O vizinho anterior a 'i'. Se 'i' for o primeiro cliente, o vizinho é o depósito
      int node_prev_i = (i == 0) ? 0 : d_tour[r_ln->start_index + i - 1];

      // O vizinho posterior a 'i'. Se 'i' for o último cliente, o vizinho é o depósito
      int node_next_i = (i == r_ln->size) ? 0 : d_tour[r_ln->start_index + i ];

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
  eval_pair_swap(uint32_t t, 
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
apply_swap_star(std::vector<std::vector<DeviceRoute>>& batch_routes,
                Instance& instance,
                ExecutionContext& ctx)
{
  
  cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;
  cvrp::sycl_engine::RoutePair* d_route_pairs = ctx.d_route_pairs;

  int* d_tour = ctx.d_mega_tour;
  DeviceRoute* d_routes = ctx.d_routes;
  cvrp::sycl_engine::BestSwap* d_best_swap = ctx.d_best_swap;
  int* d_num_pairs = ctx.d_num_pairs;
  int peak_routes = ctx.peak_routes;
  int peak_clients = ctx.peak_clients_per_tour;

  int batch_size = batch_routes.size();

  auto max_routes_batch_it = std::max_element(
    batch_routes.begin(), 
    batch_routes.end(), 
    [](const std::vector<DeviceRoute>& a, const std::vector<DeviceRoute>& b) {
      return a.size() < b.size();
    }
  );
  
  size_t max_routes = max_routes_batch_it->size();

  int max_combs = (max_routes > 1) ? (max_routes * (max_routes - 1)) / 2 : 0;

  size_t max_r_size = 0;
  for (const auto& routes : batch_routes) {
    if (!routes.empty()) {
      auto max_route_it = std::max_element(
        routes.begin(), 
        routes.end(), 
        [](const DeviceRoute& a, const DeviceRoute& b) {
          return a.size < b.size;
        }
      );
      if (max_route_it->size > max_r_size) {
        max_r_size = max_route_it->size;
      }
    }
  }

  BestSwap identity {cvrp::INF_F, -1, -1, -1, -1, -1, -1};

  size_t block_size = 32;

  {
    size_t local_bytes = max_r_size * (2 * sizeof(int) + 2 * sizeof(Top3Insertion));
    auto   local_lim   = ctx.q.get_device().get_info<sycl::info::device::local_mem_size>();

    if (local_bytes > local_lim)             throw std::runtime_error("memoria local insuficiente");
    if (max_routes > (size_t)ctx.peak_routes) throw std::runtime_error("rotas > peak_routes");

    int absolute_max_routes_combs = ctx.peak_routes * (ctx.peak_routes - 1 ) / 2;
    if (max_combs > absolute_max_routes_combs) throw std::runtime_error("pares > max_route_combs");
  }

  auto nd_main = ctx.q.submit([&] (sycl::handler& h){

    sycl::range<2> global{(size_t) batch_size, max_combs * block_size};
    sycl::range<2> local{1, block_size};

    auto tile_r = sycl::local_accessor<int, 1>(max_r_size, h);
    auto tile_rln = sycl::local_accessor<int, 1>(max_r_size, h);

    auto top3_v = sycl::local_accessor<Top3Insertion, 1>(max_r_size, h);
    auto top3_u = sycl::local_accessor<Top3Insertion, 1>(max_r_size, h);

    h.parallel_for(sycl::nd_range<2>{global, local},
    [=](sycl::nd_item<2> it){

      auto gp = it.get_group();

      const int    batch_id = it.get_group(0);        
      const int    comb_id  = it.get_group(1);
      const size_t local_id = it.get_local_id(1);

      BestSwap* best_swap_r_rln = d_best_swap + (size_t)batch_id * max_combs + comb_id;

      if (comb_id >= d_num_pairs[batch_id]) {
        if (gp.leader())
          *best_swap_r_rln = identity;
        return;
      }

      int r_i = d_route_pairs[comb_id].first;
      int r_j = d_route_pairs[comb_id].second;

      const DeviceRoute* p_routes = d_routes + (size_t)batch_id * peak_routes;
      const int* p_tour = d_tour + (size_t)batch_id * peak_clients;
      
      DeviceRoute r = p_routes[r_i];
      DeviceRoute rln = p_routes[r_j];
      
      // Verificar se rotas se sobrepõem
      if (!r.sector.overlap(rln.sector)) {
        if (gp.leader()) 
          *best_swap_r_rln = identity;
        return;
      }

      auto r_ptr = tile_r.get_multi_ptr<sycl::access::decorated::no>().get();
      auto rln_ptr = tile_rln.get_multi_ptr<sycl::access::decorated::no>().get();
      
      // Carrega o conteúdo de R para a memória local
      for (auto k = local_id; k < r.size; k += block_size)
        tile_r[k] = p_tour[r.start_index + k];
      
      // Carrega o conteúdo de R' para a memória local
      for (auto k = local_id; k < rln.size; k += block_size)
        tile_rln[k] = p_tour[rln.start_index + k];

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
  
          eval_pair_swap((uint32_t) winner, r_ptr, rln_ptr, r, rln, top3_v_ptr, top3_u_ptr, d_instance, result);
  
          *best_swap_r_rln = result;
        }
        else {
          *best_swap_r_rln = identity;
        }

      }
      
    });
  });

  ctx.q.submit([&](sycl::handler& h){

    h.depends_on(nd_main);

    sycl::range<1> global{batch_size * block_size};
    sycl::range<1> local{block_size};

    h.parallel_for(sycl::nd_range<1>{global, local},
    [=](sycl::nd_item<1> it){
      
      auto gp = it.get_group();
      const int batch_id = gp.get_group_linear_id();
      const size_t local_id = it.get_local_id(0);

      const BestSwap* p_best_swap = d_best_swap + (size_t)batch_id * max_combs;

      uint64_t k = NO_KEY;

      for (int c = local_id; c < max_combs; c += block_size){
        float cost = p_best_swap[c].total_cost;

        k = sycl::min(k, make_key(cost, c));
      }

      uint64_t winner = sycl::reduce_over_group(gp, k, sycl::minimum<uint64_t>());

      if (gp.leader() && winner != NO_KEY){

        BestSwap best = p_best_swap[uint32_t(winner)];

        if (best.total_cost < 0) {
          DeviceRoute* p_routes_b = d_routes + (size_t)batch_id * peak_routes;
          complete_swap_star(&best,
                            d_tour + (size_t)batch_id * peak_clients,
                            p_routes_b + best.r_i, p_routes_b + best.r_j, d_instance);
        }
      }

    });
    
  }).wait_and_throw();

}
} // namespace cvrp::sycl