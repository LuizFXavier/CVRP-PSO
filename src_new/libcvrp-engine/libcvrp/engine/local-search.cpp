#include <algorithm>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/engine/splitter.hpp>

namespace cvrp::local_search
{

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

void 
apply_swap_star(std::vector<Route> &routes)
{
}

void 
optimize(std::vector<int>& mega_tour, Instance& instance)
{
  auto routes = naive_split(mega_tour, instance);
}
} // namespace cvrp::local_search
