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

// Inclua <numeric> (std::iota) e <format> no topo do arquivo de testes.

namespace {

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

cvrp::sycl_engine::DeviceRoute
make_route(int start_index, int size, unsigned int total_demand)
{
  cvrp::sycl_engine::DeviceRoute route;

  route.start_index = start_index;
  route.size = size;
  route.total_demand = total_demand;

  return route;
}

struct SwapStarResult
{
  std::vector<int> tour;
  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
};

// Mesmo passo a passo do test_swap_star: carrega a instancia, copia tour e rotas para a VRAM,
// roda apply_swap_star e traz tour e rotas de volta.
SwapStarResult
run_swap_star(cvrp::Instance& inst,
              std::vector<int> mega_tour,
              std::vector<cvrp::sycl_engine::DeviceRoute> routes)
{
  cvrp::sycl_engine::ExecutionContext ctx;

  ctx.load_instance(inst);

  int* my_device_tour = ctx.d_mega_tour;
  cvrp::sycl_engine::DeviceRoute* my_device_routes = ctx.d_routes;
  cvrp::sycl_engine::Top3Insertion* my_device_top3 = ctx.d_top3_matrix;
  cvrp::sycl_engine::BestSwap* my_best_swap = ctx.d_best_swap;

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(cvrp::sycl_engine::DeviceRoute)).wait();

  cvrp::sycl_engine::local_search::apply_swap_star(routes, inst, cvrp::sycl_engine::ContextData{my_device_tour, my_device_routes, my_device_top3, my_best_swap}, ctx);

  SwapStarResult result;

  result.tour.resize(mega_tour.size());
  result.routes.resize(routes.size());

  ctx.q.memcpy(result.tour.data(), my_device_tour, result.tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(result.routes.data(), my_device_routes, result.routes.size() * sizeof(cvrp::sycl_engine::DeviceRoute)).wait();

  return result;
}

void
expect_tour(const char* test_name, const std::vector<int>& expected, const std::vector<int>& got)
{
  if (expected != got)
    throw std::runtime_error(std::format("Error: Swap star test '{}' failed! Expected {}, but got {}",
                                         test_name, vector_to_string(expected), vector_to_string(got)));
}

} // namespace

// Rotas com tamanhos diferentes (2 e 4). Pega o bug de id_v = t / r.size (o correto e' t / rln.size):
// com tamanhos iguais o erro fica escondido.
void
test_swap_star_unequal_route_sizes(){
  cvrp::Instance inst;
  inst.dimension = 7;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({16.89f, 15.16f, 1});
  inst.clients.push_back({8.41f, 5.18f, 1});
  inst.clients.push_back({10.23f, 8.10f, 1});
  inst.clients.push_back({15.68f, 6.07f, 1});
  inst.clients.push_back({9.53f, 11.67f, 1});
  inst.clients.push_back({18.16f, 10.09f, 1});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 4, 5, 6, 1, 2, 3, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 2, 2));
  routes.push_back(make_route(3, 4, 4));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("unequal route sizes", {0, 5, 2, 4, 6, 1, 3, 0}, result.tour);
}

// O melhor destino de v (e de u) NAO e' a posicao do outro cliente: vem do Top3 (k.insert_index).
// Pega erros em findTop3Locations (indexacao da tile / insert_index global) e em client_u_suce.
void
test_swap_star_insertion_other_position(){
  cvrp::Instance inst;
  inst.dimension = 9;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({16.89f, 15.16f, 1});
  inst.clients.push_back({8.41f, 5.18f, 1});
  inst.clients.push_back({10.23f, 8.10f, 1});
  inst.clients.push_back({15.68f, 6.07f, 1});
  inst.clients.push_back({9.53f, 11.67f, 1});
  inst.clients.push_back({18.16f, 10.09f, 1});
  inst.clients.push_back({5.64f, 15.12f, 1});
  inst.clients.push_back({12.37f, 5.01f, 1});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 4, 5, 2, 7, 6, 1, 8, 3, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 4, 4));
  routes.push_back(make_route(5, 4, 4));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("insertion at other position", {0, 8, 4, 5, 7, 2, 6, 1, 3, 0}, result.tour);
}

// Melhor inserção de v e' depois do ultimo cliente da rota destino (i == size em findTop3Locations,
// vizinho seguinte = deposito).
void
test_swap_star_insertion_at_route_end(){
  cvrp::Instance inst;
  inst.dimension = 8;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({2.69f, 16.95f, 1});
  inst.clients.push_back({15.28f, 5.10f, 1});
  inst.clients.push_back({9.91f, 8.99f, 1});
  inst.clients.push_back({13.03f, 15.77f, 1});
  inst.clients.push_back({1.88f, 0.57f, 1});
  inst.clients.push_back({16.72f, 8.66f, 1});
  inst.clients.push_back({15.25f, 0.04f, 1});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 6, 7, 5, 1, 2, 3, 4, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 3, 3));
  routes.push_back(make_route(4, 4, 4));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("insertion at route end", {0, 7, 2, 5, 1, 3, 4, 6, 0}, result.tour);
}

// Nenhuma troca melhora (melhor custo > 0): o tour deve ficar IGUAL. Pega a falta do 'total_cost < 0'
// no kernel de aplicacao (sem ele, aplica-se o melhor swap mesmo piorando, ou o identity com r_i = -1).
void
test_swap_star_no_improvement(){
  cvrp::Instance inst;
  inst.dimension = 7;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({2.69f, 16.95f, 1});
  inst.clients.push_back({15.28f, 5.10f, 1});
  inst.clients.push_back({9.91f, 8.99f, 1});
  inst.clients.push_back({13.03f, 15.77f, 1});
  inst.clients.push_back({1.88f, 0.57f, 1});
  inst.clients.push_back({16.72f, 8.66f, 1});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 2, 6, 5, 3, 4, 1, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 3, 3));
  routes.push_back(make_route(4, 3, 3));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("no improvement", mega_tour, result.tour);
}

// Rotas cheias (demanda 5 = capacidade) com demandas todas diferentes: toda troca estoura a capacidade.
// Sem a restricao haveria troca melhorando, entao o tour deve ficar igual por causa da capacidade.
void
test_swap_star_capacity_blocks_swap(){
  cvrp::Instance inst;
  inst.dimension = 5;
  inst.capacity = 5;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({16.89f, 15.16f, 4});
  inst.clients.push_back({8.41f, 5.18f, 2});
  inst.clients.push_back({10.23f, 8.10f, 1});
  inst.clients.push_back({15.68f, 6.07f, 3});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 3, 1, 2, 4, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 2, 5));
  routes.push_back(make_route(3, 2, 5));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("capacity blocks swap", mega_tour, result.tour);
}

// Alem do tour, a demanda total das rotas na VRAM deve ser atualizada apos o swap (complete_swap_star).
void
test_swap_star_updates_route_demand(){
  cvrp::Instance inst;
  inst.dimension = 7;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({16.89f, 15.16f, 2});
  inst.clients.push_back({8.41f, 5.18f, 1});
  inst.clients.push_back({10.23f, 8.10f, 3});
  inst.clients.push_back({15.68f, 6.07f, 3});
  inst.clients.push_back({9.53f, 11.67f, 1});
  inst.clients.push_back({18.16f, 10.09f, 3});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 4, 5, 6, 1, 2, 3, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 3, 7));
  routes.push_back(make_route(4, 3, 6));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("route demand update (tour)", {0, 4, 6, 1, 2, 3, 5, 0}, result.tour);

  std::vector<unsigned int> expected_demand = {8, 5};

  for (size_t i = 0; i < expected_demand.size(); ++i){

    if (result.routes[i].total_demand != expected_demand[i])
      throw std::runtime_error(std::format("Error: route {} demand expected {}, but got {}",
                                           i, expected_demand[i], result.routes[i].total_demand));
  }
}

// 3 rotas (3 pares). Dois pares melhoram, mas o melhor e' o par (0, 2): testa o indice do par e o argmin entre pares.
void
test_swap_star_best_pair_among_three_routes(){
  cvrp::Instance inst;
  inst.dimension = 10;
  inst.capacity = 100;
  inst.minimum_routes = 3;

  inst.clients.push_back({0.00f, 0.00f, 0});
  inst.clients.push_back({12.46f, 14.84f, 1});
  inst.clients.push_back({15.90f, 18.85f, 1});
  inst.clients.push_back({14.80f, 18.45f, 1});
  inst.clients.push_back({0.58f, 9.31f, 1});
  inst.clients.push_back({18.87f, 12.98f, 1});
  inst.clients.push_back({18.02f, 2.26f, 1});
  inst.clients.push_back({9.38f, 4.93f, 1});
  inst.clients.push_back({10.88f, 11.48f, 1});
  inst.clients.push_back({0.26f, 4.33f, 1});
  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 8, 9, 6, 1, 2, 4, 7, 3, 5, 0};

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, 3, 3));
  routes.push_back(make_route(4, 3, 3));
  routes.push_back(make_route(7, 3, 3));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("best pair among three routes", {0, 7, 8, 9, 1, 2, 4, 3, 5, 6, 0}, result.tour);
}

// 12 rotas de 3 clientes = 66 pares (mais pares que clientes e que o tamanho do work-group). O vencedor esta no
// par de indice 57. Pega: d_best_swap alocado com 'dimension' posicoes (< 66) e o laco com stride do kernel de reducao.
void
test_swap_star_many_route_pairs(){
  const float xs[] = {0.00f, 18.69f, 18.36f, 18.09f, 15.30f, 16.58f, 9.42f, 10.17f, 10.73f, 7.12f, 9.47f, 17.60f, 10.95f, 17.65f, 4.29f, 10.12f, 3.45f, 5.61f, 8.26f, 12.12f, 15.30f, 1.23f, 19.95f, 0.93f, 4.72f, 6.20f, 11.98f, 16.25f, 16.01f, 6.41f, 3.23f, 4.75f, 1.33f, 7.92f, 12.65f, 18.18f, 14.64f};
  const float ys[] = {0.00f, 17.31f, 16.01f, 2.02f, 12.31f, 16.13f, 6.86f, 8.28f, 5.53f, 14.94f, 1.79f, 1.73f, 6.00f, 16.96f, 18.55f, 3.56f, 2.12f, 10.69f, 4.05f, 13.43f, 4.44f, 4.58f, 3.91f, 12.53f, 2.06f, 12.54f, 8.62f, 0.86f, 3.87f, 11.17f, 6.10f, 6.62f, 8.03f, 3.10f, 5.53f, 11.45f, 17.09f};
  const int n_routes = 12, route_size = 3;

  cvrp::Instance inst;
  inst.dimension = 37;
  inst.capacity = 100;
  inst.minimum_routes = n_routes;

  inst.clients.push_back({xs[0], ys[0], 0});  // depósito

  for (int i = 1; i < (int) inst.dimension; ++i)
    inst.clients.push_back({xs[i], ys[i], 1});

  inst.build_distance_matrix();

  // Cada rota tem 3 clientes consecutivos: tour = {0, 1, 2, ..., 36, 0}
  std::vector<int> mega_tour(inst.dimension + 1, 0);
  std::iota(mega_tour.begin(), mega_tour.end() - 1, 0);

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;

  for (int r = 0; r < n_routes; ++r)
    routes.push_back(make_route(1 + r * route_size, route_size, route_size));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("many route pairs", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 28, 24, 25, 26, 27, 23, 29, 30, 31, 32, 33, 34, 35, 36, 0}, result.tour);
}

// 2 rotas longas (34 e 40 clientes): os lacos com stride (carga das tiles, Top3, pares (v,u)) dao varias voltas
// por work-item, e a tile/Top3 em memoria local precisa comportar a rota inteira.
void
test_swap_star_long_routes(){
  const float xs[] = {0.00f, 4.97f, 13.85f, 9.92f, 19.57f, 7.35f, 0.38f, 18.22f, 0.89f, 4.82f, 16.62f, 12.15f, 14.75f, 9.69f, 6.34f, 2.50f, 10.57f, 16.30f, 12.10f, 17.00f, 13.86f, 12.90f, 19.59f, 14.00f, 6.58f, 0.13f, 8.60f, 10.33f, 8.37f, 6.71f, 13.53f, 4.74f, 3.07f, 9.96f, 5.11f, 12.51f, 9.46f, 8.61f, 0.56f, 19.53f, 16.32f, 7.25f, 9.40f, 2.45f, 12.79f, 5.39f, 8.64f, 0.32f, 11.23f, 12.75f, 5.89f, 4.48f, 16.39f, 3.19f, 19.92f, 18.31f, 3.78f, 16.58f, 11.58f, 1.96f, 1.70f, 4.47f, 7.52f, 19.19f, 2.86f, 2.52f, 11.74f, 0.53f, 12.49f, 15.88f, 19.32f, 9.95f, 2.25f, 0.67f, 5.32f};
  const float ys[] = {0.00f, 15.65f, 9.20f, 17.75f, 17.01f, 10.33f, 8.87f, 5.42f, 10.66f, 17.99f, 11.47f, 12.70f, 19.17f, 9.31f, 6.94f, 11.41f, 9.30f, 16.35f, 16.53f, 14.23f, 16.50f, 10.39f, 6.39f, 2.12f, 12.08f, 0.18f, 18.53f, 1.86f, 16.42f, 7.56f, 1.70f, 18.53f, 16.51f, 6.03f, 10.29f, 19.33f, 19.41f, 4.54f, 6.01f, 0.99f, 9.61f, 15.50f, 8.06f, 11.19f, 0.18f, 4.19f, 10.80f, 10.00f, 3.70f, 4.85f, 13.28f, 14.52f, 17.06f, 1.00f, 13.95f, 19.25f, 19.45f, 16.06f, 8.24f, 2.74f, 6.35f, 0.54f, 4.92f, 4.98f, 6.22f, 2.28f, 8.22f, 9.88f, 13.92f, 12.86f, 1.57f, 5.32f, 1.76f, 10.36f, 18.60f};
  const int size_a = 34, size_b = 40;

  cvrp::Instance inst;
  inst.dimension = 75;
  inst.capacity = 100;
  inst.minimum_routes = 2;

  inst.clients.push_back({xs[0], ys[0], 0});  // depósito

  for (int i = 1; i < (int) inst.dimension; ++i)
    inst.clients.push_back({xs[i], ys[i], 1});

  inst.build_distance_matrix();

  // tour = {0, 1, 2, ..., 74, 0}
  std::vector<int> mega_tour(inst.dimension + 1, 0);
  std::iota(mega_tour.begin(), mega_tour.end() - 1, 0);

  std::vector<cvrp::sycl_engine::DeviceRoute> routes;
  routes.push_back(make_route(1, size_a, size_a));
  routes.push_back(make_route(1 + size_a, size_b, size_b));

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("long routes", {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 53, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 7, 70, 71, 72, 73, 74, 0}, result.tour);
}

// Mesma instancia do test_swap_star (que troca 5 e 6), mas com setores angulares DISJUNTOS: o par e' podado e o
// tour deve ficar igual. Assume que CircleSector::initialize/extend recebem o angulo polar no mesmo formato
// de import_mega_tour (ajuste os valores se a sua unidade for outra).
void
test_swap_star_sector_pruning(){
  cvrp::Instance inst;
  inst.dimension = 7;
  inst.capacity = 6;
  inst.minimum_routes = 2;

  inst.clients.push_back({0.0f, 0.0f, 0});
  inst.clients.push_back({0.0f, 3.0f, 1});
  inst.clients.push_back({3.0f, 1.0f, 1});
  inst.clients.push_back({3.0f, 0.0f, 1});
  inst.clients.push_back({1.0f, 3.0f, 1});
  inst.clients.push_back({2.0f, 1.0f, 1});
  inst.clients.push_back({1.0f, 2.0f, 1});

  inst.build_distance_matrix();

  std::vector<int> mega_tour = {0, 1, 4, 5, 6, 2, 3, 0};

  auto route1 = make_route(1, 3, 3);
  auto route2 = make_route(4, 3, 3);

  route1.sector.initialize(0);
  route1.sector.extend(100);

  route2.sector.initialize(30000);
  route2.sector.extend(30100);

  std::vector<cvrp::sycl_engine::DeviceRoute> routes = {route1, route2};

  auto result = run_swap_star(inst, mega_tour, routes);

  expect_tour("sector pruning", mega_tour, result.tour);
}

int
main()
{
  test_two_opt();
  test_route_circle_sector();
  test_swap_star();

  test_swap_star_unequal_route_sizes();
  test_swap_star_insertion_other_position();
  test_swap_star_insertion_at_route_end();
  test_swap_star_no_improvement();
  test_swap_star_capacity_blocks_swap();
  test_swap_star_updates_route_demand();
  test_swap_star_best_pair_among_three_routes();
  test_swap_star_many_route_pairs();
  test_swap_star_sector_pruning();
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
  inst.capacity = 6;
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

  cvrp::sycl_engine::DeviceRoute route1, route2;

  route1.size = 3;
  route2.size = 3;

  route1.start_index = 1;
  route2.start_index = 4;

  // route1.sector.start = 0;
  // route1.sector.end = 10;

  // route1.sector.start = 5;
  // route1.sector.end = 30;

  std::vector<cvrp::sycl_engine::DeviceRoute> routes = {route1, route2};

  int* my_device_tour = ctx.d_mega_tour;
  cvrp::sycl_engine::DeviceRoute* my_device_routes = ctx.d_routes;
  cvrp::sycl_engine::Top3Insertion* my_device_top3 = ctx.d_top3_matrix;
  cvrp::sycl_engine::BestSwap* my_best_swap = ctx.d_best_swap;

  ctx.q.memcpy(my_device_tour, mega_tour.data(), mega_tour.size() * sizeof(int)).wait();
  ctx.q.memcpy(my_device_routes, routes.data(), routes.size() * sizeof(cvrp::sycl_engine::DeviceRoute)).wait();

  cvrp::sycl_engine::local_search::apply_swap_star(routes, inst, cvrp::sycl_engine::ContextData{my_device_tour, my_device_routes, my_device_top3, my_best_swap}, ctx);

  std::vector<int> comparison = {0, 1, 4, 6, 5, 2, 3, 0};

  ctx.q.memcpy(mega_tour.data(), my_device_tour, mega_tour.size() * sizeof(int)).wait();
  

  for (int i = 0; i < comparison.size(); ++i){

    if (comparison[i] != mega_tour[i])
      throw std::runtime_error(std::format("Error: Swap star failed on first route! Expected {}, but got {}", vector_to_string(comparison), vector_to_string(mega_tour)));
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