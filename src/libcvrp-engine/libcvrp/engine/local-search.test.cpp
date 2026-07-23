#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/engine/local-search.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>
#include <libcvrp/core/CircleSector.hpp>
#include <libcvrp/core/constants.hpp>

void test_two_opt();
void test_swap_star();
void test_route_circle_sector();

int
main()
{
  test_two_opt();
  test_swap_star();
  test_route_circle_sector();
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
  inst.dimension = 6; // 1 Depósito e 5 Clientes
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

  inst.build_distance_matrix();

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

void 
test_route_circle_sector()
{
  cvrp::Instance inst;
  inst.dimension = 4; // 1 Depósito e 3 Clientes
  inst.capacity = 3;
  
  // Depósito na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  // Cliente na posição (0,1)
  inst.clients.push_back({0.0f, 1.0f, 1});

  // Cliente na posição (1,1)
  inst.clients.push_back({1.0f, 1.0f, 1});

  // Cliente na posição (1,0)
  inst.clients.push_back({1.0f, 0.0f, 1});

  // Cliente na posição (-1,1)
  inst.clients.push_back({-1.0f, 1.0f, 1});

  // Cliente na posição (-1,0)
  inst.clients.push_back({-1.0f, 0.0f, 1});

  {
    auto& clients = inst.clients;
    for (int i = 1; i < clients.size(); ++i){
      clients[i].polarAngle = cvrp::CircleSector::positive_mod(
				32768. * atan2(clients[i].y - clients[0].y, clients[i].x - clients[0].x) / cvrp::PI);
    }
  }

  // Cliente 1: Posição (0,1) - Equivalente a 90 graus
  if (inst.clients[1].polarAngle != 16383) {
    throw std::runtime_error(std::format("Error: Incorrect polar angle! Expected {}, but got {}", 16383, inst.clients[1].polarAngle));
  }

  // Cliente 2: Posição (1,1) - Equivalente a 45 graus
  if (inst.clients[2].polarAngle != 8191) {
    throw std::runtime_error(std::format("Error: Incorrect polar angle! Expected {}, but got {}", 8191, inst.clients[2].polarAngle));
  }

  // Cliente 3: Posição (1,0) - Equivalente a 0 graus (sobre o eixo X positivo)
  if (inst.clients[3].polarAngle != 0) {
    throw std::runtime_error(std::format("Error: Incorrect polar angle! Expected {}, but got {}", 0, inst.clients[3].polarAngle));
  }

  // Cliente 4: Posição (-1,1) - Equivalente a 135 graus
  if (inst.clients[4].polarAngle != 24575) {
    throw std::runtime_error(std::format("Error: Incorrect polar angle! Expected {}, but got {}", 24575, inst.clients[4].polarAngle));
  }

  // Cliente 5: Posição (-1,0) - Equivalente a 180 graus (sobre o eixo X negativo)
  if (inst.clients[5].polarAngle != 32767) {
    throw std::runtime_error(std::format("Error: Incorrect polar angle! Expected {}, but got {}", 32767, inst.clients[5].polarAngle));
  }

  std::vector<int> tour = {0, 3, 2, 1, 4, 5, 0};

  auto routes = cvrp::local_search::import_mega_tour(tour, inst);

  if (routes[0].sector.start != 0)
    throw std::runtime_error(std::format("Error: Incorrect first route circle start! Expected {}, but got {}", 0, routes[0].sector.start));

  if (routes[0].sector.end != 16383)
    throw std::runtime_error(std::format("Error: Incorrect first route circle end! Expected {}, but got {}", 16383, routes[0].sector.end));

  if (routes[1].sector.start != 24575)
    throw std::runtime_error(std::format("Error: Incorrect second route circle start! Expected {}, but got {}", 24575, routes[1].sector.start));

  if (routes[1].sector.end != 32767)
    throw std::runtime_error(std::format("Error: Incorrect second route circle end! Expected {}, but got {}", 32767, routes[1].sector.end));
}