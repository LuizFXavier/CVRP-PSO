#include <stdexcept>
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
optimize(std::vector<std::vector<int>*> mega_tours, Instance& instance, int start_id, ExecutionContext& ctx)
{

  std::vector<cvrp::sycl_engine::DeviceRoute> routes_set[2] = {import_mega_tour(*(mega_tours[0]), instance), import_mega_tour(*(mega_tours[1]), instance)};

  apply_two_opt(*(mega_tours[0]), routes_set[0], instance);
  apply_two_opt(*(mega_tours[1]), routes_set[1], instance);

  // std::cout << "P0 routes: " << routes_set[0].size() << "\n";
  // std::cout << "P1 routes: " << routes_set[1].size() << "\n";

  // throw std::runtime_error(std::format("P0: {}, P1: {}\n", routes_set[0].size(), routes_set[1].size()));

  int* my_device_tour = ctx.d_mega_tour;
  DeviceRoute* my_device_routes = ctx.d_routes;
  Top3Insertion* my_device_top3_matrix = ctx.d_top3_matrix;
  BestSwap* my_best_swap = ctx.d_best_swap;
  // ctx.q.memcpy(my_device_tour, mega_tours[0]->data(), ctx.max_clients_per_tour * sizeof(int)).wait();
  // ctx.q.memcpy(my_device_routes, routes_set[0].data(), routes_set[0].size() * sizeof(DeviceRoute)).wait();

  // ctx.q.memcpy(my_device_tour + ctx.max_clients_per_tour, mega_tours[1]->data(), ctx.max_clients_per_tour * sizeof(int)).wait();
  // ctx.q.memcpy(my_device_routes + ctx.max_routes, routes_set[1].data(), routes_set[1].size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes_set, instance, ContextData{my_device_tour, my_device_routes, my_device_top3_matrix, my_best_swap}, ctx);

  // ctx.q.memcpy(mega_tours[0]->data(), my_device_tour, ctx.max_clients_per_tour * sizeof(int)).wait();

  // ctx.q.memcpy(mega_tours[1]->data(), my_device_tour + ctx.max_clients_per_tour, ctx.max_clients_per_tour * sizeof(int)).wait();

  // Em local-search_2.cpp (dentro da função optimize)
  ctx.q.memcpy(my_device_tour, mega_tours[0]->data(), mega_tours[0]->size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes_set[0].data(), routes_set[0].size() * sizeof(DeviceRoute)).wait();

  ctx.q.memcpy(my_device_tour + ctx.max_clients_per_tour, mega_tours[1]->data(), mega_tours[1]->size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes + ctx.max_routes, routes_set[1].data(), routes_set[1].size() * sizeof(DeviceRoute)).wait();

  apply_swap_star(routes_set, instance, ContextData{my_device_tour, my_device_routes, my_device_top3_matrix, my_best_swap}, ctx);

  // O tamanho correto volta do device sem estourar o limite do Host
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

  BestSwap identity {cvrp::INF_F, -1, -1, -1, -1, -1, -1};
  BestSwap identities[2] = {identity, identity};
  ctx.q.memcpy(my_ctx_data.my_best_swap, identities, 2 * sizeof(BestSwap)).wait();

  auto [my_device_tour, my_device_routes, my_device_top3_matrix, my_best_swap] = my_ctx_data;

  cvrp::sycl_engine::DeviceInstance d_instance = ctx.d_instance;

  unsigned int instance_dimension = d_instance.dimension;

  int max_clients_per_tour = ctx.max_clients_per_tour;

  int matrix_line_size = instance_dimension * ctx.max_routes;

  int route_a_size = routes_set[0].size();
  int route_b_size = routes_set[1].size();

  int max_routes_counter = std::max(route_a_size, route_b_size);

  int max_routes = ctx.max_routes;

  // return;

  auto ev_main = ctx.q.submit([&] (sycl::handler& h){

    auto reductor_a = sycl::reduction(my_ctx_data.my_best_swap, identity, FindBestSwap());
    auto reductor_b = sycl::reduction(my_ctx_data.my_best_swap + 1, identity, FindBestSwap());

    h.parallel_for(sycl::range<2>(max_routes_counter, max_routes_counter), reductor_a, reductor_b, 
    [=](sycl::id<2> idx, auto& res_reducer_a, auto& res_reducer_b) {

      
      int r_i = idx[0];
      int r_j = idx[1];

      if (r_i < r_j){

        if (r_j >= route_a_size)
          return;

        BestSwap local_best_a = identity;
        local_best_a.r_i = r_i;
        local_best_a.r_j = r_j;

        // Pula rotas que não intersectam os setores circulares
        if (!(my_device_routes[r_i].sector.overlap(my_device_routes[r_j].sector))) return;

        int id_j = r_j * instance_dimension;

        DeviceRoute* r = my_device_routes + r_i;
        DeviceRoute* r_ln = my_device_routes + r_j;

        // Top3 inserções de v em r'
        for (int id_v = 0; id_v < my_device_routes[r_i].size; ++id_v){

          int matrix_index = (r_i * matrix_line_size) + id_j + id_v;

          int client_v = my_device_tour[my_device_routes[r_i].start_index + id_v];

          my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
                                                                  my_device_tour,
                                                                  r_ln,
                                                                  d_instance);
        }

        int start_j_top3 = my_device_routes[r_i].size;

        // Top3 inserções de u em r
        for (int id_u = 0; id_u < my_device_routes[r_j].size; ++id_u){

          int matrix_index = (r_i * matrix_line_size) + id_j + start_j_top3 + id_u;

          int client_v = my_device_tour[my_device_routes[r_j].start_index + id_u];

          my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
                                                                  my_device_tour,
                                                                  r,
                                                                  d_instance);
        }

        for (int id_v = 0; id_v < my_device_routes[r_i].size; ++ id_v){

          int client_v = my_device_tour[r->start_index + id_v];

          for (int id_u = 0; id_u < my_device_routes[r_j].size; ++ id_u){

            int client_u = my_device_tour[r_ln->start_index + id_u];
            
            if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
              continue;

            if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
              continue;

            int matrix_index_v = (r_i * matrix_line_size) + id_j + id_v;
            int matrix_index_u = (r_i * matrix_line_size) + id_j + start_j_top3 + id_u;

            // Melhor inserção de v em r', desconsiderando U da rota r'

            auto k = my_device_top3_matrix[matrix_index_v].get_best_insertion_except_client(client_u);

            // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
            auto k_ln = my_device_top3_matrix[matrix_index_u].get_best_insertion_except_client(client_v);

            int client_v_pred = (id_v == 0) ? 0 : my_device_tour[r->start_index + id_v - 1];
            int client_v_suce = (id_v == r->size - 1) ? 0 : my_device_tour[r->start_index + id_v + 1];

            int client_u_pred = (id_u == 0) ? 0 : my_device_tour[r_ln->start_index + id_u - 1];
            int client_u_suce = (id_u == r_ln->size - 1) ? 0 : my_device_tour[r_ln->start_index + id_u + 1];

            // Custo de inserção de V na exata posição de U
            float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);

            // Melhor custo de inserção de V em r'
            float v_to_r_ln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

            // Custo de inserção de U na exata posição de V
            float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
            
            // Melhor custo de inserção de U em r
            float u_to_r = sycl::fmin(swap_u_in_v, k_ln.cost)- insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

            // Atualizar a melhor troca encontrada
            if (auto c = v_to_r_ln + u_to_r; c < local_best_a.total_cost) {

              local_best_a.total_cost = c;
              
              local_best_a.v_tour_id = r->start_index + id_v;
              local_best_a.v_tour_dest = swap_v_in_u < k.cost ? r_ln->start_index + id_u : k.insert_index;
              
              local_best_a.u_tour_id = r_ln->start_index + id_u;
              local_best_a.u_tour_dest = swap_u_in_v < k_ln.cost ? r->start_index + id_v : k_ln.insert_index;
              
            }
          }
        }
        res_reducer_a.combine(local_best_a);
        // res_reducer_b.combine(identity);
      }
      else if (r_i > r_j){

        if (r_i >= route_b_size)
          return;

        BestSwap local_best_b = identity;

        int pB_i = r_j; 
        int pB_j = r_i;

        local_best_b.r_i = pB_i + max_routes;
        local_best_b.r_j = pB_j + max_routes;

        auto pB_device_routes = my_device_routes + max_routes;
        auto pB_device_tour = my_device_tour  + max_clients_per_tour;

        // Pula rotas que não intersectam os setores circulares
        if (!(pB_device_routes[pB_i].sector.overlap(pB_device_routes[pB_j].sector))) return;

        int id_j_matrix = r_j * instance_dimension;

        DeviceRoute* r = pB_device_routes + pB_i;
        DeviceRoute* r_ln = pB_device_routes + pB_j;

        // Top3 inserções de v em r'
        for (int id_v = 0; id_v < pB_device_routes[pB_i].size; ++id_v){

          // Endereçamento usa r_i (linha) e r_j (coluna)
          int matrix_index = (r_i * matrix_line_size) + id_j_matrix + id_v;

          int client_v = pB_device_tour[pB_device_routes[pB_i].start_index + id_v];

          my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
                                                                  pB_device_tour,
                                                                  r_ln,
                                                                  d_instance);
        }

        int start_j_top3 = pB_device_routes[pB_i].size;

        // Top3 inserções de u em r
        for (int id_u = 0; id_u < pB_device_routes[pB_j].size; ++id_u){

          // Endereçamento usa r_i (linha) e r_j (coluna)
          int matrix_index = (r_i * matrix_line_size) + id_j_matrix + start_j_top3 + id_u;

          int client_v = pB_device_tour[pB_device_routes[pB_j].start_index + id_u];

          my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
                                                                  pB_device_tour,
                                                                  r,
                                                                  d_instance);
        }

        for (int id_v = 0; id_v < pB_device_routes[pB_i].size; ++ id_v){

          int client_v = pB_device_tour[r->start_index + id_v];

          for (int id_u = 0; id_u < pB_device_routes[pB_j].size; ++ id_u){

            int client_u = pB_device_tour[r_ln->start_index + id_u];
            
            if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
              continue;

            if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
              continue;

            int matrix_index_v = (r_i * matrix_line_size) + id_j_matrix + id_v;
            int matrix_index_u = (r_i * matrix_line_size) + id_j_matrix + start_j_top3 + id_u;

            // Melhor inserção de v em r', desconsiderando U da rota r'

            auto k = my_device_top3_matrix[matrix_index_v].get_best_insertion_except_client(client_u);

            // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
            auto k_ln = my_device_top3_matrix[matrix_index_u].get_best_insertion_except_client(client_v);

            int client_v_pred = (id_v == 0) ? 0 : pB_device_tour[r->start_index + id_v - 1];
            int client_v_suce = (id_v == r->size - 1) ? 0 : pB_device_tour[r->start_index + id_v + 1];

            int client_u_pred = (id_u == 0) ? 0 : pB_device_tour[r_ln->start_index + id_u - 1];
            int client_u_suce = (id_u == r_ln->size - 1) ? 0 : pB_device_tour[r_ln->start_index + id_u + 1];

            // Custo de inserção de V na exata posição de U
            float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);

            // Melhor custo de inserção de V em r'
            float v_to_r_ln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

            // Custo de inserção de U na exata posição de V
            float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
            
            // Melhor custo de inserção de U em r
            float u_to_r = sycl::fmin(swap_u_in_v, k_ln.cost)- insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

            // Atualizar a melhor troca encontrada
            if (auto c = v_to_r_ln + u_to_r; c < local_best_b.total_cost) {

              local_best_b.total_cost = c;
              
              local_best_b.v_tour_id = r->start_index + id_v;
              local_best_b.v_tour_dest = swap_v_in_u < k.cost ? r_ln->start_index + id_u : k.insert_index;
              
              local_best_b.u_tour_id = r_ln->start_index + id_u;
              local_best_b.u_tour_dest = swap_u_in_v < k_ln.cost ? r->start_index + id_v : k_ln.insert_index;
              
            }
          }
        }
        res_reducer_a.combine(identity);
        // res_reducer_b.combine(local_best_b);

      }
      else{
        res_reducer_a.combine(identity);
        // res_reducer_b.combine(identity);
      }
    {  
    {
    //   BestSwap local_best = identity;

    //   local_best.r_i = r_i;
    //   local_best.r_j = r_j;

    //   // Evita avaliar a mesma rota ou processar o par espelhado duas vezes
    //   if (r_i >= r_j) return;
      
    //   // Pula rotas que não intersectam os setores circulares
    //   if (!(my_device_routes[r_i].sector.overlap(my_device_routes[r_j].sector))) return;

    //   int id_j = r_j * instance_dimension;

    //   DeviceRoute* r = my_device_routes + r_i;
    //   DeviceRoute* r_ln = my_device_routes + r_j;

    //   // Top3 inserções de v em r'
    //   for (int id_v = 0; id_v < my_device_routes[r_i].size; ++id_v){

    //     int matrix_index = (r_i * matrix_line_size) + id_j + id_v;

    //     int client_v = my_device_tour[my_device_routes[r_i].start_index + id_v];

    //     my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
    //                                                             my_device_tour,
    //                                                             r_ln,
    //                                                             d_instance);
    //   }

    //   int start_j_top3 = my_device_routes[r_i].size;

    //   // Top3 inserções de u em r
    //   for (int id_u = 0; id_u < my_device_routes[r_j].size; ++id_u){

    //     int matrix_index = (r_i * matrix_line_size) + id_j + start_j_top3 + id_u;

    //     int client_v = my_device_tour[my_device_routes[r_j].start_index + id_u];

    //     my_device_top3_matrix[matrix_index] = findTop3Locations(client_v, 
    //                                                             my_device_tour,
    //                                                             r,
    //                                                             d_instance);
    //   }

    //   for (int id_v = 0; id_v < my_device_routes[r_i].size; ++ id_v){

    //     int client_v = my_device_tour[r->start_index + id_v];

    //     for (int id_u = 0; id_u < my_device_routes[r_j].size; ++ id_u){

    //       int client_u = my_device_tour[r_ln->start_index + id_u];
          
    //       if(r->total_demand - d_instance.clients[client_v].demand + d_instance.clients[client_u].demand > d_instance.capacity)
    //         continue;

    //       if(r_ln->total_demand - d_instance.clients[client_u].demand + d_instance.clients[client_v].demand > d_instance.capacity)
    //         continue;

    //       int matrix_index_v = (r_i * matrix_line_size) + id_j + id_v;
    //       int matrix_index_u = (r_i * matrix_line_size) + id_j + start_j_top3 + id_u;

    //       // Melhor inserção de v em r', desconsiderando U da rota r'

    //       auto k = my_device_top3_matrix[matrix_index_v].get_best_insertion_except_client(client_u);

    //       // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
    //       auto k_ln = my_device_top3_matrix[matrix_index_u].get_best_insertion_except_client(client_v);

    //       int client_v_pred = (id_v == 0) ? 0 : my_device_tour[r->start_index + id_v - 1];
    //       int client_v_suce = (id_v == r->size - 1) ? 0 : my_device_tour[r->start_index + id_v + 1];

    //       int client_u_pred = (id_u == 0) ? 0 : my_device_tour[r_ln->start_index + id_u - 1];
    //       int client_u_suce = (id_u == r_ln->size - 1) ? 0 : my_device_tour[r_ln->start_index + id_u + 1];

    //       // Custo de inserção de V na exata posição de U
    //       float swap_v_in_u = insertion_cost(client_v, client_u_pred, client_u_suce, d_instance);

    //       // Melhor custo de inserção de V em r'
    //       float v_to_r_ln = sycl::fmin(swap_v_in_u, k.cost) - insertion_cost(client_v, client_v_pred, client_v_suce, d_instance);

    //       // Custo de inserção de U na exata posição de V
    //       float swap_u_in_v = insertion_cost(client_u, client_v_pred, client_v_suce, d_instance);
          
    //       // Melhor custo de inserção de U em r
    //       float u_to_r = sycl::fmin(swap_u_in_v, k_ln.cost)- insertion_cost(client_u, client_u_pred, client_u_suce, d_instance);

    //       // Atualizar a melhor troca encontrada
    //       if (auto c = v_to_r_ln + u_to_r; c < local_best.total_cost) {

    //         local_best.total_cost = c;
            
    //         local_best.v_tour_id = r->start_index + id_v;
    //         local_best.v_tour_dest = swap_v_in_u < k.cost ? r_ln->start_index + id_u : k.insert_index;
            
    //         local_best.u_tour_id = r_ln->start_index + id_u;
    //         local_best.u_tour_dest = swap_u_in_v < k_ln.cost ? r->start_index + id_v : k_ln.insert_index;
            
    //       }
    //     }
    //   }
    //   res_reducer.combine(local_best);
      }
    }
    });
    
  });

  ctx.q.submit([&] (sycl::handler& h){ 
  
    h.depends_on(ev_main);
  
    h.single_task([=]() {
  
      if (my_best_swap[0].total_cost < 0) {
  
        complete_swap_star(my_best_swap, my_device_tour, my_device_routes + my_best_swap[0].r_i, my_device_routes + my_best_swap[0].r_j, d_instance);
      }
      if (my_best_swap[1].total_cost < 0) {
      
        complete_swap_star(my_best_swap + 1, my_device_tour + max_clients_per_tour, my_device_routes +  my_best_swap[1].r_i, my_device_routes + my_best_swap[1].r_j, d_instance);
      }
    });
  });

  ctx.q.wait();

  // BestSwap host_best_swap;

  // // Faz a cópia da VRAM (Device) para a RAM (Host) e aguarda a conclusão
  // ctx.q.memcpy(&host_best_swap, my_ctx_data.my_best_swap + 1, sizeof(BestSwap)).wait();

  // // Lança a exceção formatada para depuração
  // throw std::runtime_error(std::format(
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
}
} // namespace cvrp::sycl
