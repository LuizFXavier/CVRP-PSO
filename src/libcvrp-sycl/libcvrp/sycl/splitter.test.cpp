#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/sycl/splitter.hpp>

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
  
  auto routes = cvrp::sycl_engine::naive_split(mega_tour, inst);

  if (routes.size() > 1){
    throw std::runtime_error("Error: More routes than expected were created (1 expected)!");
  }

  if (routes.size() < 1){
    throw std::runtime_error("Error: Less routes than expected were created (1 expected)!");
  }

  if (routes[0].total_demand != 9){
    throw std::runtime_error(std::format("Error: Wrong calculation of route demand! Got {}.", routes[0].total_demand));
  }

  if (routes[0].start_index != 1){
    throw std::runtime_error(std::format("Error: Wrong start index on route! Got {}.", routes[0].start_index));
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

  inst.clients.push_back({1.0f, 1.0f, 2});

  std::vector<int> mega_tour = {0, 1, 2, 3, 0};
  
  auto routes = cvrp::sycl_engine::naive_split(mega_tour, inst);

  // General result check

  if (routes.size() > 2){
    throw std::runtime_error("Error: More routes than expected were created (2 expected)!");
  }

  if (routes.size() < 2){
    throw std::runtime_error("Error: Less routes than expected were created (2 expected)!");
  }

  // First route check

  if (routes[0].size != 1){
    throw std::runtime_error(std::format("Error: Wrong number of clients on the first route! Got {}.", routes[0].size));
  }

  if (routes[0].total_demand != 5){
    throw std::runtime_error(std::format("Error: Wrong calculation of first route demand! (Got {}, expected 5).", routes[0].total_demand));
  }

  if (routes[0].start_index != 1){
    throw std::runtime_error(std::format("Error: Wrong start index on the first route! Got {}.", routes[0].start_index));
  }

  // Second route check

  if (routes[1].size != 2){
    throw std::runtime_error(std::format("Error: Wrong number of clients on the second route! Got {}.", routes[1].size));
  }

  if (routes[1].total_demand != 8){
    throw std::runtime_error(std::format("Error: Wrong calculation of second route demand! (Got {}, expected 6).", routes[1].total_demand));
  }

  if (routes[1].start_index != 2){
    throw std::runtime_error(std::format("Error: Wrong start index on the second route! Got {}.", routes[1].start_index));
  }

  return true;
}
