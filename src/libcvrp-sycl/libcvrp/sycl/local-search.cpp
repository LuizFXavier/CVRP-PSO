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
optimize(std::vector<int>& mega_tour, Instance& instance, ExecutionContext& ctx, int start_id)
{
  auto routes = import_mega_tour(mega_tour, instance);

  apply_two_opt(mega_tour, routes, instance);

  int* my_device_tour = ctx.d_swarm_mega_tours + (start_id * ctx.max_clients_per_tour);
  DeviceRoute* my_device_routes = ctx.d_swarm_routes + (start_id * ctx.max_routes_per_tour);
  Top3Insertion* my_device_top3 = ctx.d_swarm_top3 + (start_id * ctx.max_routes_per_tour);

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes, instance, my_device_tour, my_device_routes, my_device_top3, ctx);

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

  float 
  insertion_cost(int v, int p, int s, cvrp::sycl_engine::DeviceInstance instance) 
  {

    return instance.client_distance(p, v) +
           instance.client_distance(v, s) -
           instance.client_distance(p, s);
  }

  inline Top3Insertion
  findTop3Locations(int v, int* my_device_tour, DeviceRoute* r_ln, cvrp::sycl_engine::DeviceInstance instance)
  {   
    Top3Insertion top3;

    for (auto i = r_ln->start_index; i < r_ln->size; ++i){

      top3.compareAndAdd(insert_info{i, i+1, insertion_cost(v, my_device_tour[i], my_device_tour[i+1], instance)});
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
                int* my_device_tour, 
                DeviceRoute* my_device_routes,
                Top3Insertion* my_device_top3,
                ExecutionContext& ctx)
{
  auto& clients = instance.clients;

  for(unsigned i = 0; i < routes.size(); ++i){
    for(unsigned j = i+1; j < routes.size(); ++j){

      // Pula rotas que não intersectam os setores circulares
      if (!(routes[i].sector.overlap(routes[j].sector)))
        continue;

      ctx.q.submit([&] (sycl::handler& h){

        DeviceRoute* local_route = my_device_routes + i;

        h.parallel_for(sycl::range(routes[i].size), [=](sycl::id<1> idx) {

          int id_v = idx[0];

          int client_v = my_device_tour[local_route->start_index + id_v];
          
          my_device_top3[id_v] = findTop3Locations(client_v, my_device_tour, local_route, ctx.d_instance);
        });
      });

      ctx.q.submit([&] (sycl::handler& h){

        DeviceRoute* local_route = my_device_routes + j; 

        int top3_start_id = (my_device_routes + i)->size; 

        Top3Insertion* local_top3 = my_device_top3 + top3_start_id;

        h.parallel_for(sycl::range(routes[j].size), [=](sycl::id<1> idx) {

          int id_u = idx[0];

          int client_u = my_device_tour[local_route->start_index + id_u];
          
          my_device_top3[id_u] = findTop3Locations(client_u, my_device_tour, local_route, ctx.d_instance);
        });
      });

      ctx.q.wait();
      return;
      // insert_info best_v;
      // insert_info best_u;

      // unsigned best_v_id, best_u_id;

      // for (unsigned id_v = 1; id_v < routes[i].size() - 1; ++id_v){
      //   for (unsigned id_u = 1; id_u < routes[j].size() - 1; ++id_u){
            
      //     unsigned v = routes[i][id_v], u = routes[j][id_u];

      //     if(routes[i].total_demand - clients[v].demand + clients[u].demand > instance.capacity)
      //       continue;
          
      //     if(routes[j].total_demand - clients[u].demand + clients[v].demand > instance.capacity)
      //       continue;
          
      //     // Melhor inserção de v em r', fora inserção no mesmo lugar que o U e desconsiderando este da rota

      //     auto k = top3_insert_v[id_v].get_best_insertion_except_id(id_u);

      //     // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota

      //     auto k_ln = top3_insert_u[id_u].get_best_insertion_except_id(id_v);

      //     // Custo de inserção de V na exata posição de U
      //     float swap_v_in_u = insertion_cost(routes[i][id_v], routes[j][id_u -1], routes[j][id_u+1], instance);

      //     // Melhor custo de inserção de V em r'
      //     float v_to_r_ln = std::min(swap_v_in_u, k.cost) - insertion_cost(routes[i][id_v], routes[i][id_v-1], routes[i][id_v+1], instance);

      //     // Custo de inserção de U na exata posição de V
      //     float swap_u_in_v = insertion_cost(routes[j][id_u], routes[i][id_v -1], routes[i][id_v+1], instance);
          
      //     // Melhor custo de inserção de U em r
      //     float u_to_r = std::min(swap_u_in_v, k_ln.cost)- insertion_cost(routes[j][id_u], routes[j][id_u-1], routes[j][id_u+1], instance);

      //     // Atualizar a melhor troca encontrada
      //     if (auto c = v_to_r_ln + u_to_r; c < best_swap_cost) {

      //       best_swap_cost = c;
      //       best_v = swap_v_in_u < k.cost ? insert_info{id_u-1, id_u+1, swap_v_in_u} : k;
      //       best_v_id = id_v;

      //       best_u = swap_u_in_v < k_ln.cost ? insert_info{id_v-1, id_v+1, swap_u_in_v} : k_ln;
      //       best_u_id = id_u;
            
      //     }
      //   }
      // }
      // A melhor troca que otimiza a solução, caso exista, é executada
      // if (best_swap_cost < 0) {
      //   complete_swap_star(best_v_id, best_u_id, routes[i], routes[j], best_v, best_u, instance);
      // }
    }
  }
}
} // namespace cvrp::sycl