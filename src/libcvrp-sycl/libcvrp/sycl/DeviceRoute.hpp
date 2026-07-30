#pragma once

#include <libcvrp/core/CircleSector.hpp>

namespace cvrp::sycl_engine
{
struct DeviceRoute
{
  int start_index{};
  int size{};
  unsigned int total_demand{};
  CircleSector sector{};
};

} // namespace cvrp::sycl_engine
