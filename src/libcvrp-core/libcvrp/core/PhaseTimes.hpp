#pragma once
#include <vector>

namespace cvrp
{
  struct PhaseTimes
  {
    double import{};
    double two_opt{};
    double swap_star{};
    int calls_used = 0;
    bool hit_cap = false;
    std::vector<int> improved_hist;
  };
} // namespace cvrp