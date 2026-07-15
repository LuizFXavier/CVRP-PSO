#pragma once

#include <vector>

namespace cvrp
{

struct Route
{
  unsigned total_demand{};
  double cost{};
  std::vector<int> path{};

  auto& operator[](size_t i){return path[i];}

  auto size() const {return path.size();}
};
    
} // namespace cvrp

