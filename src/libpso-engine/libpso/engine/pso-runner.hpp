#pragma once

#include <functional>

#include <libcvrp/core/Instance.hpp>
#include <libpso/core/Hyperparameters.hpp>
#include <libpso/core/Particle.hpp>
#include <libpso/core/PartialCost.hpp>
#include <libpso/core/PhaseTimes.hpp>

namespace pso
{
  struct SwarmStats  { int distinct = -1; double frac_eq_gbest = -1.0; };
  struct InitInfo    { int swarm_size = 0; double best_cost = 0; double mean_cost = 0; int distinct = 0; };
  struct IterationInfo { int iter = 0; float g_best_of = 0; SwarmStats swarm; };

  struct Observers {
    std::function<void(const InitInfo&)> on_init;
    std::function<void(const IterationInfo&)> on_iteration;
    int stats_every = 10;   // a cada quantas iterações medir a diversidade (0 = nunca)
  };

  using OptimizerFunc = std::function<void(std::vector<std::vector<int>*>& tours, cvrp::Instance& instance, cvrp::PhaseTimes& times)>;
  Particle run_pso(cvrp::Instance& instance, Hyperparameters hyperparameters, OptimizerFunc optimizer, pso::PartialCost& costs, pso::PhaseTimes& times, const Observers* obs = nullptr);
} // namespace pso
