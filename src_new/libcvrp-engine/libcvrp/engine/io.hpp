#pragma once

#include <string>

#include <libcvrp/core/Instance.hpp>

namespace cvrp::io
{
  Instance read_instance(std::string instance_path);
} // namespace cvrp::io
