#pragma once

#include <sycl/sycl.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/sycl/DeviceInstance.hpp>
#include <libcvrp/sycl/Top3Insertion.hpp>

namespace cvrp::sycl_engine {

class 
ExecutionContext {

public:

  sycl::queue q; // Fila de execução atrelada ao dispositivo
  DeviceInstance d_instance; // Guarda a struct com os ponteiros
  
  int* d_swarm_mega_tours;
  DeviceRoute* d_swarm_routes;

  Top3Insertion* d_swarm_top3;

  int max_clients_per_tour;
  int max_routes_per_tour;

  ExecutionContext(int elite_size, int instance_dimension) : 
  q(sycl::default_selector_v) 
  {
    max_clients_per_tour = instance_dimension + 2; 
    max_routes_per_tour = instance_dimension;

    d_swarm_mega_tours = sycl::malloc_device<int>(elite_size * max_clients_per_tour, q);
                
    d_swarm_routes = sycl::malloc_device<DeviceRoute>(elite_size * max_routes_per_tour, q);

    d_swarm_top3 = sycl::malloc_device<Top3Insertion>(elite_size * max_routes_per_tour, q);
  }

  // Carrega a instância lida para a memória do dispositivo
  inline void 
  load_instance(const cvrp::Instance& instance) {
      
    d_instance.dimension = instance.dimension;
    d_instance.capacity = instance.capacity;

    // Alocação de memória na GPU (USM)
    d_instance.clients = sycl::malloc_device<cvrp::Client>(instance.clients.size(), q);
    d_instance.distance_matrix = sycl::malloc_device<float>(instance.distance_matrix.size(), q);

    // Copia os dados para os ponteiros da GPU
    q.memcpy(d_instance.clients, instance.clients.data(), 
              instance.clients.size() * sizeof(cvrp::Client));
              
    q.memcpy(d_instance.distance_matrix, instance.distance_matrix.data(), 
              instance.distance_matrix.size() * sizeof(float));

    q.wait();
  }
  
  // Destrutor limpa a memória da GPU
  ~ExecutionContext() 
  {
    if (d_instance.clients) sycl::free(d_instance.clients, q);
    if (d_instance.distance_matrix) sycl::free(d_instance.distance_matrix, q);

    if (d_swarm_mega_tours) sycl::free(d_swarm_mega_tours, q);
    if (d_swarm_routes) sycl::free(d_swarm_routes, q);
    if (d_swarm_top3) sycl::free(d_swarm_top3, q);
  }
};
}