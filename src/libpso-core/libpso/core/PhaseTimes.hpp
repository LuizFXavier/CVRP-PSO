#pragma once
#include <libcvrp/core/PhaseTimes.hpp>

namespace pso
{
  struct PhaseTimes
  {
    double fitness_calculation{};
    double velocity_update{};
    double position_update{};
    cvrp::PhaseTimes optimizer{};
  };
} // namespace pso