#include <algorithm>
#include <unordered_map>
#include <vector>
#include <chrono>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/engine/splitter.hpp>
#include <libcvrp/core/constants.hpp>

namespace cvrp::local_search
{

void 
optimize(std::vector<int>& mega_tour, Instance& instance, PhaseTimes& times, int swap_cap)
{
  using clk = std::chrono::steady_clock;
  auto ms = [](clk::time_point a, clk::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  };

  auto t0 = clk::now();

  auto routes = import_mega_tour(mega_tour, instance);

  auto t1 = clk::now();

  apply_two_opt(routes, instance);

  auto t2 = clk::now();

  apply_swap_star(routes, instance, times, swap_cap);

  auto t3 = clk::now();

  times.import    += ms(t0, t1);
  times.two_opt   += ms(t1, t2);
  times.swap_star += ms(t2, t3);

  int tour_index = 1;

  for (int r = 0; r < routes.size(); ++r){
    for (int j = 1; j < routes[r].size() - 1; ++j){

      mega_tour[tour_index] = routes[r].path[j];

      ++tour_index;
    }
  }
}

std::vector<cvrp::Route> 
import_mega_tour(std::vector<int> &mega_tour, Instance& instance)
{
  auto routes = naive_split(mega_tour, instance);

  auto& clients = instance.clients;

  for (auto& route : routes){
    
    route.sector.initialize(clients[route[1]].polarAngle);

    for (int i = 1; i < route.size() - 1; ++i)
      route.sector.extend(clients[route[i]].polarAngle);
  }

  return routes;
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
    unsigned pred{};
    unsigned suce{};
    float cost{};
  };

  struct Top3Insertion
  {
    insert_info best_insertions[3];
    
    void 
    compareAndAdd(insert_info candidate)
    {
      if (candidate.cost >= best_insertions[2].cost) 
        return;

      else if (candidate.cost >= best_insertions[1].cost){

        best_insertions[2] = candidate;
      }
      else if (candidate.cost >= best_insertions[0].cost){

        best_insertions[2] = best_insertions[1];
        best_insertions[1] = candidate;
      }
      else{
        best_insertions[2] = best_insertions[1];
        best_insertions[1] = best_insertions[0];
        best_insertions[0] = candidate;
      }

    }
    insert_info
    get_best_insertion_except_id(unsigned id){

      if (best_insertions[0].pred != id && best_insertions[0].suce != id)
        return best_insertions[0];

      if (best_insertions[1].pred != id && best_insertions[1].suce != id)
        return best_insertions[1];

      if (best_insertions[2].pred != id && best_insertions[2].suce != id)
        return best_insertions[2];

      return {0, 0, cvrp::INF_F};
    }
    Top3Insertion(){
      best_insertions[0].cost = cvrp::INF_F;
      best_insertions[1].cost = cvrp::INF_F;
      best_insertions[2].cost = cvrp::INF_F;
    }
  };

  float 
  insertion_cost(unsigned v, unsigned p, unsigned s, Instance& instance) 
  {
    return instance.client_distance(p, v) +
           instance.client_distance(v, s) -
           instance.client_distance(p, s);
  }

  Top3Insertion
  findTop3Locations(unsigned v, Route &r_ln, Instance& instance)
  {   
    Top3Insertion top3;

    auto cmp = [](insert_info& a, insert_info& b) {
        return a.cost < b.cost;
    };
    
    for(unsigned i = 0; i < r_ln.size() - 1; ++i){

      top3.compareAndAdd(insert_info{i, i+1, insertion_cost(v, r_ln[i], r_ln[i+1], instance)});
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
apply_swap_star(std::vector<Route>& routes, Instance& instance, PhaseTimes& times, int swap_cap)
{
  constexpr float kEps = 1e-4f;   // use o MESMO valor do kernel da GPU

  auto& clients = instance.clients;
  times.improved_hist.clear();
  times.calls_used = 0;
  times.hit_cap = false;

  if (routes.size() < 2)
    return;

  // buffers reutilizados em todos os pares (evita O(K^2) alocações por varredura)
  std::vector<Top3Insertion> top3_insert_v, top3_insert_u;

  int iter = 0;
  int n_improved = 0;

  do {
    n_improved = 0;

    for (unsigned i = 0; i + 1 < routes.size(); ++i) {
      for (unsigned j = i + 1; j < routes.size(); ++j) {

        // Poda por sobreposição de setores circulares
        if (!routes[i].sector.overlap(routes[j].sector))
          continue;

        const unsigned size_i = routes[i].size();
        const unsigned size_j = routes[j].size();
        if (size_i < 3 || size_j < 3)   // sem clientes em alguma das rotas
          continue;

        top3_insert_v.resize(size_i - 1);
        top3_insert_u.resize(size_j - 1);

        // Melhores custos de inserção de cada cliente na outra rota
        for (unsigned id_v = 1; id_v < size_i - 1; ++id_v)
          top3_insert_v[id_v] = findTop3Locations(routes[i][id_v], routes[j], instance);

        for (unsigned id_u = 1; id_u < size_j - 1; ++id_u)
          top3_insert_u[id_u] = findTop3Locations(routes[j][id_u], routes[i], instance);

        insert_info best_v, best_u;
        unsigned best_v_id = 0, best_u_id = 0;
        float best_swap_cost = -kEps;   // só aceita ganho maior que a tolerância
        bool found = false;

        for (unsigned id_v = 1; id_v < size_i - 1; ++id_v) {
          const unsigned v = routes[i][id_v];

          for (unsigned id_u = 1; id_u < size_j - 1; ++id_u) {
            const unsigned u = routes[j][id_u];

            if (routes[i].total_demand - clients[v].demand + clients[u].demand > instance.capacity)
              continue;
            if (routes[j].total_demand - clients[u].demand + clients[v].demand > instance.capacity)
              continue;

            auto k    = top3_insert_v[id_v].get_best_insertion_except_id(id_u);
            auto k_ln = top3_insert_u[id_u].get_best_insertion_except_id(id_v);

            const float swap_v_in_u =
              insertion_cost(v, routes[j][id_u - 1], routes[j][id_u + 1], instance);
            const float v_to_r_ln = std::min(swap_v_in_u, k.cost)
              - insertion_cost(v, routes[i][id_v - 1], routes[i][id_v + 1], instance);

            const float swap_u_in_v =
              insertion_cost(u, routes[i][id_v - 1], routes[i][id_v + 1], instance);
            const float u_to_r = std::min(swap_u_in_v, k_ln.cost)
              - insertion_cost(u, routes[j][id_u - 1], routes[j][id_u + 1], instance);

            const float c = v_to_r_ln + u_to_r;
            if (c < best_swap_cost) {
              best_swap_cost = c;
              best_v = swap_v_in_u < k.cost ? insert_info{id_u - 1, id_u + 1, swap_v_in_u} : k;
              best_v_id = id_v;
              best_u = swap_u_in_v < k_ln.cost ? insert_info{id_v - 1, id_v + 1, swap_u_in_v} : k_ln;
              best_u_id = id_u;
              found = true;
            }
          }
        }

        // Aplica imediatamente a melhor troca do par (atualização contínua)
        if (found) {
          complete_swap_star(best_v_id, best_u_id, routes[i], routes[j],
                             best_v, best_u, instance);   // deve atualizar total_demand e os setores
          ++n_improved;
        }
      }
    }

    times.improved_hist.push_back(n_improved);
    ++iter;

  } while (n_improved > 0 && iter < swap_cap);

  times.calls_used = iter;
  times.hit_cap = (n_improved > 0);
}

} // namespace cvrp::local_search
