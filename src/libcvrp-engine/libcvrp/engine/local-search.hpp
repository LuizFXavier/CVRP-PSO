#pragma once
#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>

namespace cvrp::local_search
{
  void optimize(std::vector<int>& mega_tour, Instance& instance);

  std::vector<cvrp::Route> import_mega_tour(std::vector<int>& mega_tour, Instance& instance);
  
  void apply_two_opt(std::vector<Route>& routes, Instance& instance);
  void apply_swap_star(std::vector<Route>& routes, Instance& instance);

} // namespace cvrp::local_search
