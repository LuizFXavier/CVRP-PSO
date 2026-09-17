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

  // 1. Pré-processamento na CPU: Agrupa pares válidos de AMBAS as partículas
  std::vector<ActivePair> active_pairs;
  active_pairs.reserve((routes_set[0].size() * (routes_set[0].size() - 1)) / 2 + 
                       (routes_set[1].size() * (routes_set[1].size() - 1)) / 2);

  for (int p = 0; p < 2; ++p) {
    for (int i = 0; i < routes_set[p].size() - 1; ++i) {
      for (int j = i + 1; j < routes_set[p].size(); ++j) {
        if (routes_set[p][i].sector.overlap(routes_set[p][j].sector)) {
          active_pairs.push_back({p, i, j});
        }
      }
    }
  }

  // Se nenhuma combinação de rota se sobrepõe, sai cedo
  if (active_pairs.empty()) return;

  // 2. Cópia das combinações ativas para a GPU
  ActivePair* d_active_pairs = sycl::malloc_device<ActivePair>(active_pairs.size(), ctx.q);
  ctx.q.memcpy(d_active_pairs, active_pairs.data(), active_pairs.size() * sizeof(ActivePair)).wait();

  cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;
  unsigned int instance_dimension = d_instance.dimension;

  int tours_p_offset = ctx.max_clients_per_tour;
  int routes_p_offset = ctx.max_routes;
  int top3_comb_offset = instance_dimension;

  // 3. Execução do Kernel em Grid 1D
  auto ev_main = ctx.q.submit([&] (sycl::handler & h){

    auto reductor = sycl::reduction(my_ctx_data.my_dual_best_swap, identity_dual, FindDualBestSwap());

    h.parallel_for(sycl::range<1>(active_pairs.size()), reductor,
    [=](sycl::id<1> idx, auto& res_reducer){

      DualBestSwap thread_result = { identity_swap, identity_swap };

      int id_part = d_active_pairs[idx[0]].particle_id;
      int r_i = d_active_pairs[idx[0]].r_i;
      int r_j = d_active_pairs[idx[0]].r_j;

      BestSwap local_best = identity_swap;
      local_best.r_i = r_i;
      local_best.r_j = r_j;

      // Cálculo direto de ponteiros usando offsets
      int* particle_d_tour = my_device_tour + (id_part * tours_p_offset);
      DeviceRoute* particle_d_routes = my_device_routes + (id_part * routes_p_offset);
      
      // Cada thread no Grid 1D ganha seu próprio espaço exclusivo no d_top3_vector
      Top3Insertion* particle_d_top3_vector = my_device_top3_vector + (idx[0] * top3_comb_offset);

      DeviceRoute* r = particle_d_routes + r_i;
      DeviceRoute* r_ln = particle_d_routes + r_j;

      // --- Início do Swap Star (Lógica original inalterada) ---
      for (int id_v = 0; id_v < r->size; ++id_v){
        int client_v = particle_d_tour[r->start_index + id_v];
        particle_d_top3_vector[id_v] = findTop3Locations(client_v, particle_d_tour, r_ln, d_instance);
      }

      int start_j_top3 = r->size;

      for (int id_u = 0; id_u < r_ln->size; ++id_u){
        int client_u = particle_d_tour[r_ln->start_index + id_u];
        particle_d_top3_vector[start_j_top3 + id_u] = findTop3Locations(client_u, particle_d_tour, r, d_instance);
      }

      for (int id_v = 0; id_v < r->size; ++id_v){
        int client_v = particle_d_tour[r->start_index + id_v];

        for (int id_u = 0; id_u < r_ln->size; ++id_u){
          int client_u = particle_d_tour[r_ln->start_index + id_u];
          
          if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
            continue;

          if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
            continue;

          auto k = particle_d_top3_vector[id_v].get_best_insertion_except_client(client_u);
          auto k_ln = particle_d_top3_vector[start_j_top3 + id_u].get_best_insertion_except_client(client_v);

          int client_v_pred = (id_v == 0) ? 0 : particle_d_tour[r->start_index + id_v - 1];
          int client_v_suce = (id_v == r->size - 1) ? 0 : particle_d_tour[r->start_index + id_v + 1];

          int client_u_pred = (id_u == 0) ? 0 : particle_d_tour[r_ln->start_index + id_u - 1];
          int client_u_suce = (id_u == r_ln->size - 1) ? 0 : particle_d_tour[r_ln->start_index + id_u + 1];

          float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);
          float v_to_r_ln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

          float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
          float u_to_r = sycl::fmin(swap_u_in_v, k_ln.cost) - insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

          if (auto c = v_to_r_ln + u_to_r; c < local_best.total_cost) {
            local_best.total_cost = c;
            local_best.v_tour_id = r->start_index + id_v;
            local_best.v_tour_dest = swap_v_in_u < k.cost ? r_ln->start_index + id_u : k.insert_index;
            local_best.u_tour_id = r_ln->start_index + id_u;
            local_best.u_tour_dest = swap_u_in_v < k_ln.cost ? r->start_index + id_v : k_ln.insert_index;
          }
        }
      }
      
      // Atribuição de acordo com a partícula a qual a thread atual pertence
      if (id_part == 0) {
        thread_result.pA = local_best;
      } else {
        thread_result.pB = local_best;
      }

      res_reducer.combine(thread_result);
    });
  });

  // 4. Efetiva as trocas encontradas
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
  
  // Limpa o USM utilizado no pré-processamento
  sycl::free(d_active_pairs, ctx.q);
}
} // namespace cvrp::sycl