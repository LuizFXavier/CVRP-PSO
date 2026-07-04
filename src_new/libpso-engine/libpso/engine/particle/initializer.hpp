#pragma once
#include <random>

#include <libcvrp/core/Instance.hpp>
#include <libpso/core/Particle.hpp>

namespace pso
{
  Particle random_initialize(int dimension, std::mt19937& gen);
  
  Particle sector_initialize(cvrp::Instance& instance);
} // namespace pso
