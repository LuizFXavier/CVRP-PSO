#include <stdexcept>
#include <cstdlib>

#include <libpso/engine/pso-runner.hpp>

#include <libcvrp/core/Instance.hpp>

using namespace std;

cvrp::Instance create_simple_instance();

bool test_trivial_instance();
bool test_zero_iterations();
bool test_reproducible();

int 
main() 
{
  if (!test_trivial_instance()) {
    throw runtime_error("Error: The PSO failed on resolving a trivial instance.");
  }
  if (!test_zero_iterations()) {
    throw runtime_error("Error: O PSO não lidou bem com zero iterações.");
  }
  if (!test_reproducible()) {
    throw runtime_error("Erro: O PSO produziu resultados diferentes para a mesma semente.");
  }
  return 0;
}

// Cria uma instância mínima para os testes
cvrp::Instance 
create_simple_instance() 
{
  cvrp::Instance inst;
  inst.dimension = 2; // 1 Depósito e 1 Cliente
  inst.capacity = 10;
  
  // Depósito na origem (0,0)
  inst.clients.push_back({0.0f, 0.0f, 0});

  // Cliente na posição (3,4). Distância de 5
  inst.clients.push_back({3.0f, 4.0f, 5});
  
  return inst;
}

bool 
test_trivial_instance() 
{
  auto inst = create_simple_instance();
  
  pso::Hyperparameters hp;
  hp.swarm_size = 10;
  hp.iterations = 5;
  hp.w_max = 0.9;
  hp.w_min = 0.4;
  hp.cognitive_c = 1.0;
  hp.social_c = 1.0;
  hp.elite_size = 2;

  pso::Particle best;
  try
  {
    best = pso::run_pso(inst, hp);
  }
  catch(const std::exception& e)
  {
    return false;
  }

  // A distância mínima teórica é ir ao cliente e voltar (5 + 5 = 10)
  // O PSO deve encontrar um valor finito.
  return best.curr_of < cvrp::INF_F;
}

bool 
test_zero_iterations() 
{
  auto inst = create_simple_instance();
  pso::Hyperparameters hp;
  hp.swarm_size = 5;
  hp.iterations = 0; // O loop não deve correr

  // Uma partícula não nula deve ser devolvida pelo PSO
  pso::Particle best = pso::run_pso(inst, hp);

  return true; 
}

bool 
test_reproducible() {

  auto inst = create_simple_instance();
  pso::Hyperparameters hp;
  hp.swarm_size = 10;
  hp.iterations = 5;
  hp.elite_size = 2;

  // Primeira execução
  srand(42); 
  pso::Particle best1 = pso::run_pso(inst, hp);

  // Segunda execução com a mesma semente
  srand(42);
  pso::Particle best2 = pso::run_pso(inst, hp);

  return best1.curr_of == best2.curr_of;
}