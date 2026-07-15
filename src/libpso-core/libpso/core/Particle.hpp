#pragma once
#include <vector>

#include <libcvrp/core/constants.hpp>
#include  <libpso/core/Velocity.hpp>

namespace pso
{

struct Particle{
  
  float best_of {cvrp::INF_F};
  float curr_of {cvrp::INF_F};

  std::vector<int> best_solution{};
  std::vector<int> curr_solution{};
  
  Velocity velocity;
};
} // namespace pso
