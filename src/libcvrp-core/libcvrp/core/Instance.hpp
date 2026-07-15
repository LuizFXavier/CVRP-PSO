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

  std::vector<std::vector<float>> distance_matrix{{}};
};

} // namespace cvrp
