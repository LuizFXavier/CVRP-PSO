#pragma once

#include <libcvrp/core/Instance.hpp>
#include <libpso/core/Hyperparameters.hpp>
#include <libpso/core/Particle.hpp>

namespace pso
{
  Particle run_pso(cvrp::Instance& instance, Hyperparameters hyperparameters);
} // namespace pso
