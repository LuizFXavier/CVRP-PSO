// TODO: Adaptar os testes para o uso do SYCL
#include <vector>
#include <stdexcept>
#include <format>

#include <libcvrp/sycl/local-search.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/Route.hpp>
#include <libcvrp/core/CircleSector.hpp>
#include <libcvrp/core/constants.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp>

void test_two_opt();
void test_swap_star();
void test_route_circle_sector();

int
main()
{
  test_two_opt();
  test_route_circle_sector();
  test_swap_star();
  return 0;
}

std::string 
vector_to_string(const std::vector<int>& vec) {
    std::string result = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        result += std::to_string(vec[i]);
        if (i < vec.size() - 1) {
            result += ", ";
        }
    }
    result += "]";
    return result;
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

  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 2, 1, 3, 0};

  cvrp::sycl_engine::DeviceRoute route;

  route.size = 3;
  route.start_index = 1;
 
  std::vector<cvrp::sycl_engine::DeviceRoute> routes = {route};

  cvrp::sycl_engine::local_search::apply_two_opt(mega_tour, routes, inst);

  std::vector<int> comparison = {0, 1, 2, 3, 0};

  for (int i = 0; i < comparison.size(); ++i){

    if (comparison[i] != mega_tour[i])
      throw std::runtime_error(std::format("Error: two-opt did not produce correct output! Expected {}, but got {}", 
                                            vector_to_string(comparison), vector_to_string(mega_tour)));
  }
}

void 
test_swap_star(){
  cvrp::Instance inst;
  inst.dimension = 7; // 1 Depósito e 5 Clientes
  inst.capacity = 10;
  inst.minimum_routes = 2; 
  
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

  cvrp::sycl_engine::ExecutionContext ctx;

  ctx.load_instance(inst);

  std::vector<int> mega_tour = {0, 1, 4, 5, 6, 2, 3, 0};
  std::vector<int> mega_tour_b = {0, 1, 4, 5, 6, 2, 3, 0};

  cvrp::sycl_engine::DeviceRoute route1, route2;

  route1.size = 3;
  route2.size = 3;

  route1.start_index = 1;
  route2.start_index = 4;

  std::vector<cvrp::sycl_engine::DeviceRoute> routes = {route1, route2};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes_b = {route1, route2};

  int* my_device_tour = ctx.d_mega_tour;
  cvrp::sycl_engine::DeviceRoute* my_device_routes = ctx.d_routes;
  cvrp::sycl_engine::Top3Insertion* my_device_top3 = ctx.d_top3_matrix;
  cvrp::sycl_engine::BestSwap* my_best_swap = ctx.d_best_swap;

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(cvrp::sycl_engine::DeviceRoute)).wait();

  ctx.q.memcpy(my_device_tour + ctx.max_clients_per_tour, mega_tour_b.data(), mega_tour_b.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes + ctx.max_routes, routes_b.data(), routes_b.size() * sizeof(cvrp::sycl_engine::DeviceRoute)).wait();

  std::vector<cvrp::sycl_engine::DeviceRoute> v_routes[2] = {routes, routes_b};

  cvrp::sycl_engine::local_search::apply_swap_star(v_routes, inst, cvrp::sycl_engine::ContextData{my_device_tour, my_device_routes, my_device_top3, my_best_swap}, ctx);

  std::vector<int> comparison = {0, 1, 4, 6, 5, 2, 3, 0};

  ctx.q.memcpy(mega_tour.data(), my_device_tour, mega_tour.size() * sizeof(int)).wait();

  ctx.q.memcpy(mega_tour_b.data(), my_device_tour + ctx.max_clients_per_tour, mega_tour.size() * sizeof(int)).wait();
  

  for (int i = 0; i < comparison.size(); ++i){

    if (comparison[i] != mega_tour[i])
      throw std::runtime_error(std::format("Error: Swap star failed on first route! Expected {}, but got {}", vector_to_string(comparison), vector_to_string(mega_tour)));

    if (comparison[i] != mega_tour_b[i])
      throw std::runtime_error(std::format("Error: Swap star failed on tour b! Expected {}, but got {}", vector_to_string(comparison), vector_to_string(mega_tour_b)));
  }
 
}

void 
test_route_circle_sector()
{
  cvrp::Instance inst;
  inst.dimension = 4; // 1 Depósito e 3 Clientes
  inst.capacity = 3;
  inst.minimum_routes = 2;
  
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

  auto routes = cvrp::sycl_engine::local_search::import_mega_tour(tour, inst);

  if (routes[0].sector.start != 0)
    throw std::runtime_error(std::format("Error: Incorrect first route circle start! Expected {}, but got {}", 0, routes[0].sector.start));

  if (routes[0].sector.end != 16383)
    throw std::runtime_error(std::format("Error: Incorrect first route circle end! Expected {}, but got {}", 16383, routes[0].sector.end));

  if (routes[1].sector.start != 24575)
    throw std::runtime_error(std::format("Error: Incorrect second route circle start! Expected {}, but got {}", 24575, routes[1].sector.start));

  if (routes[1].sector.end != 32767)
    throw std::runtime_error(std::format("Error: Incorrect second route circle end! Expected {}, but got {}", 32767, routes[1].sector.end));
}