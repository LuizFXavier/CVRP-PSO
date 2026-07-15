
#include <libcvrp/engine/splitter.hpp>

#include <libcvrp/core/Client.hpp>

namespace cvrp
{

std::vector<Route>
naive_split(std::vector<int> &mega_tour, Instance& instance)
{
  std::vector<Route> routes{};
  
  int n_routes{};

  int curr_capacity = instance.capacity;

  auto& clients = instance.clients;

  // Primeira rota
  routes.push_back(Route());
  routes[0].path.push_back(0);

  for(int i = 0; i < mega_tour.size() - 1; i++){

    if (clients[mega_tour[i+1]].demand <= curr_capacity){

      curr_capacity -= clients[mega_tour[i+1]].demand;
      routes[n_routes].path.push_back(mega_tour[i+1]);
      routes[n_routes].cost += distance(clients[mega_tour[i]], clients[mega_tour[i+1]]);
      routes[n_routes].total_demand += clients[mega_tour[i+1]].demand;
    }
    else {

      // Encerramento da rota
      curr_capacity = instance.capacity;

      // Retorno ao depósito
      routes[n_routes].path.push_back(0);
      routes[n_routes].cost += distance(clients[mega_tour[i]], clients[0]);

      // Incialização da próxima rota
      routes.push_back(Route());
      ++n_routes;

      routes[n_routes].path.push_back(0);

      curr_capacity -= clients[mega_tour[i+1]].demand;
      routes[n_routes].path.push_back(mega_tour[i+1]);
      routes[n_routes].cost += distance(clients[0], clients[mega_tour[i+1]]);
      routes[n_routes].total_demand += clients[mega_tour[i+1]].demand;
    }
  }

  return routes;
}
} // namespace cvrp

