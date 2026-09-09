#pragma once

#include <functional>

#include <libcvrp/core/Instance.hpp>
#include <libpso/core/Hyperparameters.hpp>
#include <libpso/core/Particle.hpp>

namespace pso
{
  using OptimizerFunc = std::function<void(std::vector<std::vector<int>*> /* tours */, cvrp::Instance& /*instance */, int /* particle_id */)>;
  Particle run_pso(cvrp::Instance& instance, Hyperparameters hyperparameters, OptimizerFunc optimizer);
} // namespace pso
