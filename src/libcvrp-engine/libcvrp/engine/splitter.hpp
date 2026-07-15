#pragma once
#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>

namespace cvrp
{
  std::vector<Route> naive_split(std::vector<int>& mega_tour, Instance& instance);
  
} // namespace cvrp
