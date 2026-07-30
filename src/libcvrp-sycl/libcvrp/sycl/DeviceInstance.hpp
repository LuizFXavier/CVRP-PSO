#pragma once

#include <libcvrp/core/Client.hpp>

namespace cvrp::sycl_engine
{

struct DeviceInstance {
  unsigned int dimension;
  unsigned int capacity;
  cvrp::Client* clients;         // Ponteiro para memória USM Device
  float* distance_matrix;        // Ponteiro para memória USM Device
  
  inline float client_distance(int from, int to) const {
    return distance_matrix[from * dimension + to];
  }
};  
} // namespace cvrp::sycl_engine
