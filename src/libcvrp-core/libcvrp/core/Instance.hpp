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
};

} // namespace cvrp
