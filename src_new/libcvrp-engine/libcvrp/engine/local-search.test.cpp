#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>

void test_two_opt();
void test_swap_star();

int
main()
{
  test_two_opt();
  test_swap_star();
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

void 
test_swap_star(){
  cvrp::Instance inst;
  inst.dimension = 4; // 1 Depósito e 3 Clientes
  inst.capacity = 10;
  
  // Depósito A na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  // Cliente B/1
  inst.clients.push_back({0.0f, 3.0f, 1});

  // Cliente C/2
  inst.clients.push_back({3.0f, 1.0f, 1});

  // Cliente D/3
  inst.clients.push_back({3.0f, 0.0f, 1});

  // Cliente E/4
  inst.clients.push_back({1.0f, 3.0f, 1});

  // Cliente F/5
  inst.clients.push_back({2.0f, 1.0f, 1});

  // Cliente G/6
  inst.clients.push_back({1.0f, 2.0f, 1});

  cvrp::Route route1, route2;

  route1.path = {0, 1, 4, 5, 0};
  route2.path = {0, 6, 2, 3, 0};

  std::vector<cvrp::Route> routes = {route1, route2};

  cvrp::local_search::apply_swap_star(routes, inst);

  std::vector<int> comparison1 = {0, 1, 4, 6, 0};
  std::vector<int> comparison2 = {0, 5, 2, 3, 0};

  for (int i = 0; i < comparison1.size(); ++i){

    if (comparison1[i] != routes[0].path[i])
      throw std::runtime_error(std::format("Error: Swap star failed on first route! Expected {}, but got {}", comparison1, routes[0].path));
  }

  for (int i = 0; i < comparison2.size(); ++i){

    if (comparison2[i] != routes[1].path[i])
      throw std::runtime_error(std::format("Error: Swap star failed on second route! Expected {}, but got {}", comparison2, routes[1].path));
  }
}