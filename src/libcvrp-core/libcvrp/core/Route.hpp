#pragma once

#include <vector>

#include <libcvrp/core/CircleSector.hpp>

namespace cvrp
{

struct Route
{
  unsigned total_demand{};
  double cost{};
  std::vector<int> path{};

  CircleSector sector{};

  auto& operator[](size_t i){return path[i];}

  auto size() const {return path.size();}
};
    
} // namespace cvrp

