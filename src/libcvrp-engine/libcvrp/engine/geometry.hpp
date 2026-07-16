#pragma once

#include <vector>

#include <libcvrp/core/Client.hpp>

namespace cvrp
{
  std::vector<float> compute_distances(std::vector<Client>& clients);
  
} // namespace cvrp
