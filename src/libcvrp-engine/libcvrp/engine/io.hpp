#pragma once

#include <string>
#include <vector>

#include <libcvrp/core/Instance.hpp>

namespace cvrp::io
{
  Instance read_instance(std::string instance_path);

  void save_routes(std::vector<int>& mega_tour, cvrp::Instance& instance, std::string output_path, std::string file_name);
} // namespace cvrp::io
