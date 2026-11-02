#pragma once

#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/core/PhaseTimes.hpp>
#include <libcvrp/sycl/DeviceRoute.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp>

namespace cvrp::sycl_engine::local_search
{
  void optimize(std::vector<std::vector<int>*>& batch_tours, Instance& instance, ExecutionContext& ctx, PhaseTimes& times, int swap_cap = 1);
  
  std::vector<DeviceRoute> import_mega_tour(std::vector<int>& mega_tour, Instance& instance);
  
  void apply_two_opt(std::vector<int>& mega_tour, std::vector<DeviceRoute>& routes, cvrp::Instance& instance);

  int apply_swap_star( std::vector<std::vector<DeviceRoute>>& batch_routes,
                        Instance& instance,
                        ExecutionContext& ctx,
                        double& time);
} // namespace cvrp::sycl
