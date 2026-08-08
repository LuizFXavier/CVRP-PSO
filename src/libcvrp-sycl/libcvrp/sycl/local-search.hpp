#pragma once

#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/sycl/DeviceRoute.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp>

namespace cvrp::sycl_engine::local_search
{
  void optimize(std::vector<int>& mega_tour, Instance& instance, int start_id, ExecutionContext& ctx);
  
  std::vector<DeviceRoute> import_mega_tour(std::vector<int>& mega_tour, Instance& instance);
  
  void apply_two_opt(std::vector<int>& mega_tour, std::vector<DeviceRoute>& routes, cvrp::Instance& instance);

  void apply_swap_star( std::vector<DeviceRoute>& routes,
                        Instance& instance, 
                        ContextData my_ctx_data,
                        ExecutionContext& ctx);
} // namespace cvrp::sycl
