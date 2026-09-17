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
optimize(std::vector<std::vector<int>*> mega_tours, Instance& instance, int start_id, ExecutionContext& ctx)
{
  // auto routes = import_mega_tour(mega_tour, instance);

  std::vector<cvrp::sycl_engine::DeviceRoute> routes_set[2] = {import_mega_tour(*(mega_tours[0]), instance), import_mega_tour(*(mega_tours[1]), instance)};

  // apply_two_opt(mega_tour, routes, instance);

  apply_two_opt(*(mega_tours[0]), routes_set[0], instance);
  apply_two_opt(*(mega_tours[1]), routes_set[1], instance);

  int* my_device_tour = ctx.d_mega_tour;
  DeviceRoute* my_device_routes = ctx.d_routes;
  Top3Insertion* my_device_top3_vector = ctx.d_top3_vector;
  DualBestSwap* my_dual_best_swap = ctx.d_dual_best_swap;

  ctx.q.memcpy(my_device_tour, mega_tours[0]->data(), mega_tours[0]->size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes_set[0].data(), routes_set[0].size() * sizeof(DeviceRoute)).wait();

  ctx.q.memcpy(my_device_tour + ctx.max_clients_per_tour, mega_tours[1]->data(), mega_tours[1]->size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes + ctx.max_routes, routes_set[1].data(), routes_set[1].size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes_set, instance, ContextData{my_device_tour, my_device_routes, my_device_top3_vector, my_dual_best_swap}, ctx);

  // ctx.q.memcpy(mega_tour.data(), my_device_tour, mega_tour.size() * sizeof(int)).wait();

  ctx.q.memcpy(mega_tours[0]->data(), my_device_tour, mega_tours[0]->size() * sizeof(int)).wait();
  ctx.q.memcpy(mega_tours[1]->data(), my_device_tour + ctx.max_clients_per_tour, mega_tours[1]->size() * sizeof(int)).wait();
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
apply_swap_star(std::vector<cvrp::sycl_engine::DeviceRoute> routes_set[2], 
                Instance& instance,
                ContextData my_ctx_data,
                ExecutionContext& ctx)
{
  auto& clients = instance.clients;

  auto [my_device_tour, my_device_routes, my_device_top3_vector, my_dual_best_swap] = my_ctx_data;

  BestSwap identity_swap {cvrp::INF_F, -1, -1, -1, -1, -1, -1};

  DualBestSwap identity_dual {identity_swap, identity_swap};

  ctx.q.memcpy(my_dual_best_swap, &identity_dual, sizeof(DualBestSwap)).wait();

  cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;

  cvrp::sycl_engine::RoutePair* d_route_pairs = ctx.d_route_pairs;

  unsigned int instance_dimension = d_instance.dimension;

  int tours_p_offset = ctx.max_clients_per_tour;

  int routes_p_offset = ctx.max_routes;

  int top3_p_offset = ctx.max_route_combs * instance_dimension;

  int top3_comb_offset = instance_dimension;

  int routes_size[2] = {(int)routes_set[0].size(), (int)routes_set[1].size()};

  int max_routes_counter = std::max(routes_size[0], routes_size[0]);

  int routes_combs_counter[2] = {routes_size[0] * (routes_size[0] - 1) / 2, routes_size[1] * (routes_size[1] - 1) / 2};

  int* d_routes_combs_counter = sycl::malloc_device<int>(2, ctx.q);

  ctx.q.memcpy(d_routes_combs_counter, routes_combs_counter, 2 * sizeof(int)).wait();

  int total_route_combs = (max_routes_counter * (max_routes_counter - 1)) / 2;

  int simultaneous_proc = ctx.simultaneous_proc;

  auto ev_main = ctx.q.submit([&] (sycl::handler & h){

    auto reductor = sycl::reduction(my_ctx_data.my_dual_best_swap, identity_dual, FindDualBestSwap());

    h.parallel_for(sycl::range<2>(simultaneous_proc, total_route_combs), reductor,
    [=](sycl::id<2> idx, auto& res_reducer){

      DualBestSwap thread_result = { identity_swap, identity_swap };

      int id_part = idx[0];
      int id_comb = idx[1];

      if (id_comb > d_routes_combs_counter[id_part])
        return;

      int r_i = d_route_pairs[id_comb].first;
      int r_j = d_route_pairs[id_comb].second;

      BestSwap local_best = identity_swap;

      local_best.r_i = r_i;
      local_best.r_j = r_j;

      // Pula rotas que não intersectam os setores circulares
      if (!(my_device_routes[r_i].sector.overlap(my_device_routes[r_j].sector))) return;

      int id_j = r_j * instance_dimension;

      int* particle_d_tour = my_device_tour + (id_part * tours_p_offset);
      Top3Insertion* particle_d_top3_vector = my_device_top3_vector + (id_part * top3_p_offset) + (id_comb * top3_comb_offset);

      DeviceRoute* particle_d_routes = my_device_routes + (id_part * routes_p_offset);

      DeviceRoute* r = particle_d_routes + r_i;
      DeviceRoute* r_ln = particle_d_routes + r_j;

      // Top3 inserções de v em r'
      for (int id_v = 0; id_v < r->size; ++id_v){

        int top3_vector_index = id_v;

        int client_v = particle_d_tour[r->start_index + id_v];

        particle_d_top3_vector[top3_vector_index] = findTop3Locations(client_v, 
                                                                      particle_d_tour,
                                                                      r_ln,
                                                                      d_instance);
      }

      int start_j_top3 = r->size;

      // Top3 inserções de u em r
      for (int id_u = 0; id_u < r_ln->size; ++id_u){

        int top3_vector_index = start_j_top3 + id_u;

        int client_u = particle_d_tour[r_ln->start_index + id_u];

        particle_d_top3_vector[top3_vector_index] = findTop3Locations(client_u,
                                                                      particle_d_tour,
                                                                      r,
                                                                      d_instance);
      }

      for (int id_v = 0; id_v < r->size; ++id_v){

        int client_v = particle_d_tour[r->start_index + id_v];

        for (int id_u = 0; id_u < r_ln->size; ++id_u){

          int client_u = particle_d_tour[r_ln->start_index + id_u];
          
          if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
            continue;

          if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
            continue;

          int top3_vector_index_v = id_v;
          int top3_vector_index_u = start_j_top3 + id_u;

          // Melhor inserção de v em r', desconsiderando U da rota r'

          auto k = particle_d_top3_vector[top3_vector_index_v].get_best_insertion_except_client(client_u);

          // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
          auto k_ln = particle_d_top3_vector[top3_vector_index_u].get_best_insertion_except_client(client_v);

          int client_v_pred = (id_v == 0) ? 0 : particle_d_tour[r->start_index + id_v - 1];
          int client_v_suce = (id_v == r->size - 1) ? 0 : particle_d_tour[r->start_index + id_v + 1];

          int client_u_pred = (id_u == 0) ? 0 : particle_d_tour[r_ln->start_index + id_u - 1];
          int client_u_suce = (id_u == r_ln->size - 1) ? 0 : particle_d_tour[r_ln->start_index + id_u + 1];

          // Custo de inserção de V na exata posição de U
          float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);

          // Melhor custo de inserção de V em r'
          float v_to_r_ln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

          // Custo de inserção de U na exata posição de V
          float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
          
          // Melhor custo de inserção de U em r
          float u_to_r = sycl::fmin(swap_u_in_v, k_ln.cost)- insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

          // Atualizar a melhor troca encontrada
          if (auto c = v_to_r_ln + u_to_r; c < local_best.total_cost) {

            local_best.total_cost = c;
            
            local_best.v_tour_id = r->start_index + id_v;
            local_best.v_tour_dest = swap_v_in_u < k.cost ? r_ln->start_index + id_u : k.insert_index;
            
            local_best.u_tour_id = r_ln->start_index + id_u;
            local_best.u_tour_dest = swap_u_in_v < k_ln.cost ? r->start_index + id_v : k_ln.insert_index;
            
          }
        }
      }
      
      if (id_part == 0) {
        thread_result.pA = local_best;
      } 
      else {
        thread_result.pB = local_best;
      }

      res_reducer.combine(thread_result);
    });
  });

  ctx.q.submit([&] (sycl::handler& h){ 

    h.depends_on(ev_main);

    h.single_task([=]() {
    
      if (my_dual_best_swap->pA.total_cost < 0) {

        BestSwap* pA_best_swap = &(my_dual_best_swap->pA);

        complete_swap_star(pA_best_swap, 
                           my_device_tour, 
                           my_device_routes + pA_best_swap->r_i, 
                           my_device_routes + pA_best_swap->r_j, 
                           d_instance);
      }
      if (my_dual_best_swap->pB.total_cost < 0) {

        BestSwap* pB_best_swap = &(my_dual_best_swap->pB);
      
        complete_swap_star(pB_best_swap, 
                           my_device_tour + tours_p_offset, 
                           my_device_routes + routes_p_offset + pB_best_swap->r_i, 
                           my_device_routes + routes_p_offset + pB_best_swap->r_j, 
                           d_instance);
      }
    });
  });

  ctx.q.wait();

  if (d_routes_combs_counter) sycl::free(d_routes_combs_counter, ctx.q);

  BestSwap host_best_swap;

// 2. Faz a cópia da VRAM (Device) para a RAM (Host) e aguarda a conclusão
// ctx.q.memcpy(&host_best_swap, my_ctx_data.my_dual_best_swap, sizeof(BestSwap)).wait();

//   throw std::runtime_error(std::format(
//     "\n=== DEBUG EXCEPTION: MELHOR SWAP ENCONTRADO ===\n"
//     "Custo Total (Delta): {}\n"
//     "Rota origem de V (r_i): {}\n"
//     "Rota origem de U (r_j): {}\n"
//     "Cliente V - Índice Atual: {} | Índice Destino: {}\n"
//     "Cliente U - Índice Atual: {} | Índice Destino: {}\n"
//     "===============================================\n",
//     host_best_swap.total_cost,
//     host_best_swap.r_i,
//     host_best_swap.r_j,
//     host_best_swap.v_tour_id, host_best_swap.v_tour_dest,
//     host_best_swap.u_tour_id, host_best_swap.u_tour_dest
// ));

  // sycl::free(d_valid_pairs, ctx.q);
}
} // namespace cvrp::sycl