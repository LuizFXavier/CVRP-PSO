#include <sycl/sycl.hpp>
#include <algorithm>

#include <libcvrp/sycl/local-search.hpp>

#include <libcvrp/core/constants.hpp>
#include <libcvrp/sycl/splitter.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp>
#include <libcvrp/sycl/Top3Insertion.hpp>

namespace cvrp::sycl_engine::local_search
{
void 
optimize(std::vector<int>& mega_tour, Instance& instance, int start_id, ExecutionContext& ctx)
{
  auto routes = import_mega_tour(mega_tour, instance);

  apply_two_opt(mega_tour, routes, instance);

  int* my_device_tour = ctx.d_swarm_mega_tours + (start_id * ctx.max_clients_per_tour);
  DeviceRoute* my_device_routes = ctx.d_swarm_routes + (start_id * ctx.max_routes_per_tour);
  Top3Insertion* my_device_top3 = ctx.d_swarm_top3 + (start_id * ctx.max_routes_per_tour);
  BestSwap* my_best_swap = ctx.d_best_swap + (start_id);

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes, instance, ContextData{my_device_tour, my_device_routes, my_device_top3, my_best_swap}, ctx);

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
//   void 
//   complete_swap_star(unsigned id_v, unsigned id_u, Route &r, Route &r_ln, insert_info v_insert, insert_info u_insert, Instance& instance)
//   {
//     auto& clients = instance.clients;

//     unsigned dest_v, dest_u;
//     unsigned v = r[id_v];
//     unsigned u = r_ln[id_u];

//     double rm_v = insertion_cost(v, r[id_v-1], r[id_v+1], instance);
//     double rm_u = insertion_cost(u, r_ln[id_u-1], r_ln[id_u+1], instance);

//     auto dummy_r = r.cost;
//     auto dummy_l = r_ln.cost;

//     r.cost = r.cost - rm_v;// + u_insert.cost;
//     r_ln.cost = r_ln.cost - rm_u;// + v_insert.cost;

//     r.cost += u_insert.cost;
//     r_ln.cost += v_insert.cost;

//     r.total_demand = r.total_demand - clients[v].demand + clients[u].demand;
//     r_ln.total_demand = r_ln.total_demand - clients[u].demand + clients[v].demand;

//     if(v_insert.pred < id_u){
//       dest_v = v_insert.pred + 1;
//       for(int i = id_u; i > dest_v; --i){
//         r_ln[i] = r_ln[i-1];
//       }
//     }
//     else if(v_insert.pred > id_u){
//       dest_v = v_insert.pred;
//       for (int i = id_u; i < dest_v; ++i) {
//         r_ln[i] = r_ln[i+1];
//       }
//     }
//     else{
//       dest_v = id_u;
//     }

//     if(u_insert.pred < id_v){
//       dest_u = u_insert.pred + 1;
//       for(int i = id_v; i > dest_u; --i){
//         r[i] = r[i-1];
//       }
//     }
//     else if(u_insert.pred > id_v){
//       dest_u = u_insert.pred;
//       for (int i = id_v; i < dest_u; ++i) {
//         r[i] = r[i+1];
//       }
//     }
//     else{
//       dest_u = id_v;
//     }

//     r[dest_u] = u;
//     r_ln[dest_v] = v;
// }
}

void 
apply_swap_star(std::vector<DeviceRoute>& routes, 
                Instance& instance, 
                // int* my_device_tour, 
                // DeviceRoute* my_device_routes,
                // Top3Insertion* my_device_top3,
                // BestSwap* my_best_swap,
                ContextData my_ctx_data,
                ExecutionContext& ctx)
{
  auto& clients = instance.clients;

  for(unsigned i = 0; i < routes.size(); ++i){
    for(unsigned j = i+1; j < routes.size(); ++j){

      // Pula rotas que não intersectam os setores circulares
      if (!(routes[i].sector.overlap(routes[j].sector)))
        continue;

      cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;

      ctx.q.submit([&] (sycl::handler& h){

        DeviceRoute* source_route = my_ctx_data.my_device_routes + i;
        DeviceRoute* target_route = my_ctx_data.my_device_routes + j;

        h.parallel_for(sycl::range(routes[i].size), [=](sycl::id<1> idx) {

          int id_v = idx[0];

          int client_v = my_ctx_data.my_device_tour[source_route->start_index + id_v];
          
          my_ctx_data.my_device_top3[id_v] = findTop3Locations(client_v, my_ctx_data.my_device_tour, target_route, d_instance);
        });
      });

      ctx.q.submit([&] (sycl::handler& h){

        DeviceRoute* source_route = my_ctx_data.my_device_routes + j;
        DeviceRoute* target_route = my_ctx_data.my_device_routes + i;

        // Os top3 da rota r' estão deslocados até o tamanho da rota r
        int top3_start_id = routes[i].size;

        Top3Insertion* local_top3 = my_ctx_data.my_device_top3 + top3_start_id;

        h.parallel_for(sycl::range(routes[j].size), [=](sycl::id<1> idx) {

          int id_u = idx[0];

          int client_u = my_ctx_data.my_device_tour[source_route->start_index + id_u];
          
          local_top3[id_u] = findTop3Locations(client_u, my_ctx_data.my_device_tour, target_route, d_instance);
        });
      });
      
      BestSwap identity {cvrp::INF_F, -1, -1, -1, -1};

      

      ctx.q.memcpy(my_ctx_data.my_best_swap, &identity, sizeof(BestSwap)).wait();

      ctx.q.wait();

      DeviceRoute* r = my_ctx_data.my_device_routes + i;
      DeviceRoute* r_ln = my_ctx_data.my_device_routes + j;

      ctx.q.submit([&] (sycl::handler& h){

        int top3_r_ln_start_id = routes[i].size;;

        Top3Insertion* r_ln_top3 = my_ctx_data.my_device_top3 + top3_r_ln_start_id;

        auto reductor = sycl::reduction(my_ctx_data.my_best_swap, identity, FindBestSwap());

        // loop paralelo na rota[i]
        h.parallel_for(sycl::range(routes[i].size), reductor, [=](sycl::id<1> idx, auto& res_reducer) {

          BestSwap local_best = identity;

          int id_v = idx[0];
          int client_v = my_ctx_data.my_device_tour[r->start_index + id_v];

          for (int id_u = 0; id_u < r_ln->size; ++id_u){

            int client_u = my_ctx_data.my_device_tour[r_ln->start_index + id_u];

            if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
              continue;

            if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
              continue;

            // Melhor inserção de v em r', desconsiderando U da rota r'

            auto k = my_ctx_data.my_device_top3[id_v].get_best_insertion_except_client(client_u);

            // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
            auto k_ln = r_ln_top3[id_u].get_best_insertion_except_client(client_v);

            int client_v_pred = (id_v == 0) ? 0 : my_ctx_data.my_device_tour[r->start_index + id_v - 1];
            int client_v_suce = (id_v == r->size - 1) ? 0 : my_ctx_data.my_device_tour[r->start_index + id_v + 1];

            int client_u_pred = (id_u == 0) ? 0 : my_ctx_data.my_device_tour[r_ln->start_index + id_u - 1];
            int client_u_suce = (id_u == r_ln->size - 1) ? 0 : my_ctx_data.my_device_tour[r_ln->start_index + id_u + 1];

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

          // Redução do melhor valor encontrado
          res_reducer.combine(local_best);

        });
      }).wait();
      
      //A melhor troca que otimiza a solução, caso exista, é executada
      ctx.q.single_task([=]() {
    
        if (my_ctx_data.my_best_swap->total_cost < 0) {
            
          complete_swap_star(my_ctx_data.my_best_swap, my_ctx_data.my_device_tour, r, r_ln, d_instance);
        }
      }).wait();
    }
  }
}
} // namespace cvrp::sycl