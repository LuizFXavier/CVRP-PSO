#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>

void test_two_opt();

int
main()
{
  test_two_opt();
  return 0;
}

void 
test_two_opt()
{
  cvrp::Instance inst;
  inst.dimension = 4; // 1 Depósito e 3 Clientes
  inst.capacity = 10;
  
  // Depósito na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  // Cliente na posição (1,0)
  inst.clients.push_back({1.0f, 0.0f, 1});

  // Cliente na posição (1,1)
  inst.clients.push_back({1.0f, 1.0f, 1});

  // Cliente na posição (0,1)
  inst.clients.push_back({0.0f, 1.0f, 1});

  cvrp::Route route;

  route.path = {0, 2, 1, 3, 0};

  std::vector<cvrp::Route> routes = {route};

  cvrp::local_search::apply_two_opt(routes, inst);

  std::vector<int> comparison = {0, 1, 2, 3, 0};

  for (int i = 0; i < comparison.size(); ++i){

    if (comparison[i] != routes[0].path[i])
      throw std::runtime_error(std::format("Error: two-opt did not produce correct output! Expected {}, but got {}", comparison, routes[0].path));
  }
}