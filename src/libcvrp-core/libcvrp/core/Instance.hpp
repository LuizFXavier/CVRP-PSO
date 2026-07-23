#pragma once
#include <string>
#include <vector>

#include <libcvrp/core/Client.hpp>

namespace cvrp
{

struct Instance
{ 
  std::string name{};
  unsigned int dimension{};
  unsigned int capacity{};
  
  std::vector<cvrp::Client> clients{};

  std::vector<float> distance_matrix{};

  inline float 
  client_distance(int from, int to)
  {
    return distance_matrix[from * clients.size() + to];
  }

  inline void
  build_distance_matrix()
  {
    std::vector<float> distances(clients.size() * clients.size());

    for (int i = 0; i < clients.size(); ++i){
      for (int j = 0; j < clients.size(); ++j){
        distances[i * clients.size() + j] = distance(clients[i], clients[j]);
      }
    }
    
    distance_matrix = std::move(distances);
  }
};

} // namespace cvrp
