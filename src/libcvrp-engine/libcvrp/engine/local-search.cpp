#include <algorithm>
#include <unordered_map>
#include <vector>
#include <omp.h>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/engine/splitter.hpp>
#include <libcvrp/core/constants.hpp>

namespace cvrp::local_search
{

void 
optimize(std::vector<int>& mega_tour, Instance& instance)
{
  auto routes = naive_split(mega_tour, instance);

  apply_two_opt(routes, instance);

  apply_swap_star(routes, instance);

  int tour_index = 1;

  for (int r = 0; r < routes.size(); ++r){
    for (int j = 1; j < routes[r].size() - 1; ++j){

      mega_tour[tour_index] = routes[r].path[j];

      ++tour_index;
    }
  }
}

void 
apply_two_opt(std::vector<Route> &routes, Instance& instance)
{
  auto& clients = instance.clients;
  
  for(auto& route : routes){
    
    bool improved = true;

    while (improved)
    {
      improved = false;
      for(size_t i = 1; i < route.size() - 1; ++i)
        for (size_t j = i+1; j < route.size() - 1; ++j)
        {
          // Calcula a distância antes da troca
          double old_dist = distance(clients[route[i-1]], clients[route[i]]) + distance(clients[route[j]], clients[route[j+1]]);

          // Distância após a troca (2-opt swap)
          double new_dist = distance(clients[route[i-1]], clients[route[j]]) + distance(clients[route[i]], clients[route[(j+1)]]);
          
          // Se a nova distância for menor, aplica a troca
          if (new_dist < old_dist) {

            std::reverse(route.path.begin() + i, route.path.begin() + j + 1);

            route.cost = route.cost - old_dist + new_dist;
            improved = true;
          }
        }
          
    }
  }
}

// Namespace anônimo para as funções auxiliares do swap star
namespace
{
  struct insert_info{
    unsigned pred;
    unsigned suce;
    float cost;
  };

  float 
  insertion_cost(unsigned v, unsigned p, unsigned s, Instance& instance) 
  {
    auto& clients = instance.clients;

    return distance(clients[p], clients[v]) +
           distance(clients[v], clients[s]) -
           distance(clients[p], clients[s]);
  }

  std::vector<insert_info>
  findTop3Locations(unsigned v, Route &r_ln, Instance& instance)
  {   
    std::vector<insert_info> top3;

    auto cmp = [](insert_info& a, insert_info& b) {
        return a.cost < b.cost;
    };
    
    for(unsigned i = 0; i < r_ln.size() - 1; ++i){
      if(top3.empty()){
        top3.emplace_back(insert_info{i, i+1, insertion_cost(v, r_ln[i], r_ln[i+1], instance)});
      }
      else if (top3.size() < 3) {
        top3.push_back(insert_info{i, i+1, insertion_cost(v, r_ln[i], r_ln[i+1], instance)});
        std::sort(top3.begin(), top3.end(), cmp);
      }
      else if (auto c = insertion_cost(v, r_ln[i], r_ln[i+1], instance); c < top3.back().cost) {
        top3.back() = insert_info{i, i+1, c};
        std::sort(top3.begin(), top3.end(), cmp);
      }
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
  complete_swap_star(unsigned id_v, unsigned id_u, Route &r, Route &r_ln, insert_info v_insert, insert_info u_insert, Instance& instance)
  {
    auto& clients = instance.clients;

    unsigned dest_v, dest_u;
    unsigned v = r[id_v];
    unsigned u = r_ln[id_u];

    double rm_v = insertion_cost(v, r[id_v-1], r[id_v+1], instance);
    double rm_u = insertion_cost(u, r_ln[id_u-1], r_ln[id_u+1], instance);

    auto dummy_r = r.cost;
    auto dummy_l = r_ln.cost;

    r.cost = r.cost - rm_v;// + u_insert.cost;
    r_ln.cost = r_ln.cost - rm_u;// + v_insert.cost;

    r.cost += u_insert.cost;
    r_ln.cost += v_insert.cost;

    r.total_demand = r.total_demand - clients[v].demand + clients[u].demand;
    r_ln.total_demand = r_ln.total_demand - clients[u].demand + clients[v].demand;

    if(v_insert.pred < id_u){
      dest_v = v_insert.pred + 1;
      for(int i = id_u; i > dest_v; --i){
        r_ln[i] = r_ln[i-1];
      }
    }
    else if(v_insert.pred > id_u){
      dest_v = v_insert.pred;
      for (int i = id_u; i < dest_v; ++i) {
        r_ln[i] = r_ln[i+1];
      }
    }
    else{
      dest_v = id_u;
    }

    if(u_insert.pred < id_v){
      dest_u = u_insert.pred + 1;
      for(int i = id_v; i > dest_u; --i){
        r[i] = r[i-1];
      }
    }
    else if(u_insert.pred > id_v){
      dest_u = u_insert.pred;
      for (int i = id_v; i < dest_u; ++i) {
        r[i] = r[i+1];
      }
    }
    else{
      dest_u = id_v;
    }

    r[dest_u] = u;
    r_ln[dest_v] = v;
}
}

void 
apply_swap_star(std::vector<Route> &routes, Instance& instance)
{
  auto& clients = instance.clients;

  for(unsigned i = 0; i < routes.size()-1; ++i){
    for(unsigned j = i+1; j < routes.size(); ++j){

      std::unordered_map<unsigned, std::vector<insert_info>> top3_insert_v;
      std::unordered_map<unsigned, std::vector<insert_info>> top3_insert_u;

      // Pré-processar melhores custos de inserção:
      for (unsigned id_v = 1; id_v < routes[i].size()-1; ++id_v)
        top3_insert_v[id_v] = findTop3Locations(routes[i][id_v], routes[j], instance);

      for (unsigned id_u = 1; id_u < routes[j].size()-1; ++id_u)
        top3_insert_u[id_u] = findTop3Locations(routes[j][id_u], routes[i], instance);

      double best_swap_cost = 0;

      insert_info best_v;
      insert_info best_u;

      unsigned best_v_id, best_u_id;

      for (unsigned id_v = 1; id_v < routes[i].size()-1; ++id_v){
        for (unsigned id_u = 1; id_u < routes[j].size()-1; ++id_u){
            
          unsigned v = routes[i][id_v], u = routes[j][id_u];

          if(routes[i].total_demand - clients[v].demand + clients[u].demand > instance.capacity)
            continue;
          
          if(routes[j].total_demand - clients[u].demand + clients[v].demand > instance.capacity)
            continue;
          
          // Melhor inserção de v em r', fora inserção no mesmo lugar que o U e desconsiderando este da rota
          auto k = std::min_element(top3_insert_v[id_v].begin(), top3_insert_v[id_v].end(),
                                    [&id_u](insert_info& a, insert_info& b) {
                                      if (a.pred == id_u || a.suce == id_u) return false;
                                      if (b.pred == id_u || b.suce == id_u) return true;
                                      return (a.cost < b.cost);
                                    });

          k->cost = (k->suce != id_u && k->pred != id_u) ? k->cost : cvrp::INF_F;

          // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
          auto k_ln = std::min_element(top3_insert_u[id_u].begin(), top3_insert_u[id_u].end(),
                                      [&id_v](insert_info& a, insert_info& b) {
                                          if (a.pred == id_v || a.suce == id_v) return false;
                                          if (b.pred == id_v || b.suce == id_v) return true;
                                          return (a.cost < b.cost);
                                      });

          k_ln->cost = (k_ln->suce != id_v && k_ln->pred != id_v) ? k_ln->cost : cvrp::INF_F;

          float swap_v_in_u = insertion_cost(routes[i][id_v], routes[j][id_u -1], routes[j][id_u+1], instance);

          // Melhor custo de inserção de V em r'
          float v_to_r_ln = std::min(swap_v_in_u, k->cost) - insertion_cost(routes[i][id_v], routes[i][id_v-1], routes[i][id_v+1], instance);

          // Melhor custo de inserção de U em r
          float swap_u_in_v = insertion_cost(routes[j][id_u], routes[i][id_v -1], routes[i][id_v+1], instance);

          
          float u_to_r = std::min(swap_u_in_v, k_ln->cost)- insertion_cost(routes[j][id_u], routes[j][id_u-1], routes[j][id_u+1], instance);

          // Atualizar a melhor troca encontrada
          if (auto c = v_to_r_ln + u_to_r; c < best_swap_cost) {

            best_swap_cost = c;
            best_v = swap_v_in_u < k->cost ? insert_info{id_u-1, id_u+1, swap_v_in_u} : *k;
            best_v_id = id_v;

            best_u = swap_u_in_v < k_ln->cost ? insert_info{id_v-1, id_v+1, swap_u_in_v} : *k_ln;
            best_u_id = id_u;
            
          }
        }
      }
      // A melhor troca que otimiza a solução, caso exista, é executada
      if (best_swap_cost < 0) {
        complete_swap_star(best_v_id, best_u_id, routes[i], routes[j], best_v, best_u, instance);
      }
    }
  }
}


} // namespace cvrp::local_search
