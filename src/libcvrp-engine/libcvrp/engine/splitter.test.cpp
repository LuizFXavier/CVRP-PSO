#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/engine/splitter.hpp>

#include <libcvrp/core/Instance.hpp>

bool test_naive_single_route();
bool test_naive_two_routes();

int
main(){
  test_naive_single_route();
  test_naive_two_routes();
  
  return 0;
}

bool 
test_naive_single_route()
{
  cvrp::Instance inst;
  inst.dimension = 3; // 1 Depósito e 2 Clientes
  inst.capacity = 10;
  
  // Depósito na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  inst.clients.push_back({0.0f, 1.0f, 5}); 

  inst.clients.push_back({1.0f, 0.0f, 4});

  std::vector<int> mega_tour = {0, 1, 2, 0};
  
  auto routes = cvrp::naive_split(mega_tour, inst);

  if (routes.size() > 1){
    throw std::runtime_error("Error: More routes than expected were created (1 expected)!");
  }

  if (routes.size() < 1){
    throw std::runtime_error("Error: Less routes than expected were created (1 expected)!");
  }

  if (routes[0].total_demand != 9){
    throw std::runtime_error(std::format("Error: Wrong calculation of route demand! Got {}.", routes[0].total_demand));
  }

  if (routes[0].cost > 4 || routes[0].cost < 3){
    throw std::runtime_error(std::format("Error: Wrong calculation of route cost! Got {}.", routes[0].cost));
  }

  if (routes[0].path.size() != 4){
    throw std::runtime_error(std::format("Error: Wrong number of clients on route! Got {}.", routes[0].path.size()));
  }

  return true;
}

bool 
test_naive_two_routes()
{
  cvrp::Instance inst;
  inst.dimension = 3; // 1 Depósito e 2 Clientes
  inst.capacity = 10;
  
  // Depósito na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  inst.clients.push_back({0.0f, 1.0f, 5}); 

  inst.clients.push_back({1.0f, 0.0f, 6});

  std::vector<int> mega_tour = {0, 1, 2, 0};
  
  auto routes = cvrp::naive_split(mega_tour, inst);

  // General result check

  if (routes.size() > 2){
    throw std::runtime_error("Error: More routes than expected were created (2 expected)!");
  }

  if (routes.size() < 2){
    throw std::runtime_error("Error: Less routes than expected were created (2 expected)!");
  }

  // First route check

  if (routes[0].total_demand != 5){
    throw std::runtime_error(std::format("Error: Wrong calculation of first route demand! (Got {}, expected 5).", routes[0].total_demand));
  }

  if (routes[0].cost > 2){
    throw std::runtime_error(std::format("Error: Wrong calculation of first route cost! Got {}.", routes[0].cost));
  }

  if (routes[0].path.size() != 3){
    throw std::runtime_error(std::format("Error: Wrong number of clients on the first route! Got {}.", routes[0].path.size()));
  }

  // Second route check

  if (routes[1].total_demand != 6){
    throw std::runtime_error(std::format("Error: Wrong calculation of second route demand! (Got {}, expected 6).", routes[1].total_demand));
  }

  if (routes[1].cost > 2){
    throw std::runtime_error(std::format("Error: Wrong calculation of second route cost! Got {}.", routes[1].cost));
  }

  if (routes[1].path.size() != 3){
    throw std::runtime_error(std::format("Error: Wrong number of clients on the second route! Got {}.", routes[1].path.size()));
  }

  return true;
}
