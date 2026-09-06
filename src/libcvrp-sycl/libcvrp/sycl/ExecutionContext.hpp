#pragma once

#include <sycl/sycl.hpp>

#include <libcvrp/core/Instance.hpp>
#include <libcvrp/sycl/DeviceInstance.hpp>
#include <libcvrp/sycl/DeviceRoute.hpp>
#include <libcvrp/sycl/Top3Insertion.hpp>
#include <libcvrp/sycl/RoutePair.hpp>

namespace cvrp::sycl_engine {

struct ContextData
{
  int* my_device_tour;
  DeviceRoute* my_device_routes;
  Top3Insertion* my_device_top3_matrix;
  BestSwap* my_best_swap;
};


class 
ExecutionContext {

public:

  sycl::queue q; // Fila de execução atrelada ao dispositivo
  DeviceInstance d_instance; // Guarda a struct com os ponteiros

  int* d_mega_tour;

  DeviceRoute* d_routes;

  Top3Insertion* d_top3_matrix;

  BestSwap* d_best_swap;

  RoutePair* d_route_pairs;

  int max_clients_per_tour;
  
  int max_routes;

  ExecutionContext()
  : q(sycl::default_selector_v) {}

  // Carrega a instância lida para a memória do dispositivo
  inline void 
  load_instance(const cvrp::Instance& instance) {
      
    d_instance.dimension = instance.clients.size();
    d_instance.capacity = instance.capacity;
    d_instance.minimum_routes = instance.minimum_routes;

    // Alocação de memória na GPU (USM)
    d_instance.clients = sycl::malloc_device<cvrp::Client>(instance.clients.size(), q);
    d_instance.distance_matrix = sycl::malloc_device<float>(instance.distance_matrix.size(), q);

    // Copia os dados para os ponteiros da GPU
    q.memcpy(d_instance.clients, instance.clients.data(), 
              instance.clients.size() * sizeof(cvrp::Client));
              
    q.memcpy(d_instance.distance_matrix, instance.distance_matrix.data(), 
              instance.distance_matrix.size() * sizeof(float));

    // Alocação dos dados para as buscas locais

    max_routes = d_instance.minimum_routes * 2;

    max_clients_per_tour = instance.dimension + 1;

    d_mega_tour = sycl::malloc_device<int>(max_clients_per_tour, q);
    
    d_routes = sycl::malloc_device<DeviceRoute>(max_routes, q);
    
    d_top3_matrix = sycl::malloc_device<Top3Insertion>(max_routes * (max_routes * d_instance.dimension), q);

    d_best_swap = sycl::malloc_device<BestSwap>(d_instance.dimension, q);

    int max_route_combs = (max_routes * (max_routes - 1)) / 2;

    d_route_pairs = sycl::malloc_device<RoutePair>(max_route_combs, q);

    setup_route_combs(d_instance.minimum_routes, max_routes); 

    q.wait();
  }

  void setup_route_combs(int minimum_routes, int max_routes);
  
  // Destrutor limpa a memória da GPU
  ~ExecutionContext() 
  {
    if (d_instance.clients) sycl::free(d_instance.clients, q);
    if (d_instance.distance_matrix) sycl::free(d_instance.distance_matrix, q);

    if (d_mega_tour) sycl::free(d_mega_tour, q);
    if (d_routes) sycl::free(d_routes, q);
    if (d_top3_matrix) sycl::free(d_top3_matrix, q);

    if (d_best_swap) sycl::free(d_best_swap, q);

    if (d_route_pairs) sycl::free(d_route_pairs, q);
  }
};
}