#pragma once

#include <vector>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/sycl/DeviceRoute.hpp>

namespace cvrp::sycl_engine
{
  std::vector<DeviceRoute> naive_split(std::vector<int>& mega_tour, Instance& instance);
} // namespace cvrp::sycl_engine
