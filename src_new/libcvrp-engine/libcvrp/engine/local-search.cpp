#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/engine/splitter.hpp>

namespace cvrp::local_search
{

void 
apply_two_opt(std::vector<Route> &routes)
{
}

void 
apply_swap_star(std::vector<Route> &routes)
{
}

void 
optimize(std::vector<int>& mega_tour, Instance& instance)
{
  auto routes = naive_split(mega_tour, instance);
}
} // namespace cvrp::local_search
