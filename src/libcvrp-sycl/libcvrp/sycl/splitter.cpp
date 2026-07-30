
#include <libcvrp/sycl/splitter.hpp>

#include <libcvrp/core/Client.hpp>

namespace cvrp::sycl_engine
{

std::vector<DeviceRoute>
naive_split(std::vector<int> &mega_tour, Instance& instance)
{
  std::vector<DeviceRoute> routes{};
  
  int n_routes{};

  int curr_capacity = instance.capacity;

  auto& clients = instance.clients;

  int start_index = 1;

  // Primeira rota
  routes.push_back(DeviceRoute());
  routes[0].start_index = start_index;
  routes[0].total_demand = clients[mega_tour[1]].demand;
  routes[0].size += 1;

  curr_capacity -= clients[mega_tour[1]].demand;

  // Loop pelo mega_tour, ignorando os depósitos inicial e final presentes
  for(int i = 1; i < mega_tour.size() - 2; i++){

    if (clients[mega_tour[i+1]].demand <= curr_capacity){

      curr_capacity -= clients[mega_tour[i+1]].demand;
      
      routes[n_routes].total_demand += clients[mega_tour[i+1]].demand;

      routes[n_routes].size += 1;
      
    }
    else {

      // Encerramento da rota
      curr_capacity = instance.capacity;

      // Incialização da próxima rota
      routes.push_back(DeviceRoute());
      ++n_routes;

      routes[n_routes].start_index = i+1;

      curr_capacity -= clients[mega_tour[i+1]].demand;
      
      routes[n_routes].total_demand += clients[mega_tour[i+1]].demand;

      routes[n_routes].size += 1;
    }
  }

  return routes;
}
} // namespace cvrp::sycl_engine

