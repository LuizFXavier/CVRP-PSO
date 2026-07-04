#pragma once
#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>

namespace cvrp::local_search
{
  void apply_two_opt(std::vector<Route>& routes);
  void apply_swap_star(std::vector<Route>& routes);

  void optimize(std::vector<int>& mega_tour, Instance& instance);

} // namespace cvrp::local_search
