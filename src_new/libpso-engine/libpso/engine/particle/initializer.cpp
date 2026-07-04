#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

#include <libpso/engine/particle/initializer.hpp>

namespace pso
{
  
Particle 
random_initialize(int dimension, std::mt19937& gen)
{
  Particle particle;

  particle.curr_solution.resize(dimension + 1);

  for(int i = 0; i < dimension; i++){
    particle.curr_solution[i] = i;
  }

  particle.curr_solution[dimension] = particle.curr_solution[0];

    
  std::shuffle(particle.curr_solution.begin() + 1, particle.curr_solution.end() - 1, gen);
  
  return particle;
}

} // namespace pso
