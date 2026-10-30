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
  BestSwap* my_best_swap;
};


class 
ExecutionContext {

public:

  sycl::queue q; // Fila de execução atrelada ao dispositivo
  DeviceInstance d_instance{}; // Guarda a struct com os ponteiros

  int* d_mega_tour{};

  DeviceRoute* d_routes{};

  BestSwap* d_best_swap{};

  RoutePair* d_route_pairs{};

  int* d_num_pairs{};

  int peak_clients_per_tour{};
  
  int peak_routes{};

  ExecutionContext()
  : q(sycl::default_selector_v,
      [](sycl::exception_list errs) {
        for (auto& e : errs) {
          try { 
            std::rethrow_exception(e); 
          }
          catch (const sycl::exception& ex) { 
            std::cerr << "SYCL: " << ex.what() << "\n"; 
            std::abort(); 
          }
        }
      },
      sycl::property_list{sycl::property::queue::enable_profiling()}) {}

  inline
  void setup(const cvrp::Instance& instance, int batch_size)
  {

    load_instance(instance);

    this->d_mega_tour = sycl::malloc_device<int>(peak_clients_per_tour * batch_size, q);

    this->d_routes = sycl::malloc_device<DeviceRoute>(peak_routes * batch_size, q);

    int max_route_combs = (peak_routes * (peak_routes - 1)) / 2;

    this->d_best_swap = sycl::malloc_device<BestSwap>(max_route_combs * batch_size, q);

    this->d_route_pairs = sycl::malloc_device<RoutePair>(max_route_combs, q);
    
    this->d_num_pairs = sycl::malloc_device<int>(batch_size, q);

    setup_route_combs(d_instance.minimum_routes, peak_routes);
  }

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

    this->peak_routes = d_instance.minimum_routes * 2;

    this->peak_clients_per_tour = instance.dimension + 1;

    q.wait();
  }
  
  // Destrutor limpa a memória da GPU
  ~ExecutionContext() 
  {
    if (d_instance.clients) sycl::free(d_instance.clients, q);
    if (d_instance.distance_matrix) sycl::free(d_instance.distance_matrix, q);

    if (d_mega_tour) sycl::free(d_mega_tour, q);
    if (d_routes) sycl::free(d_routes, q);

    if (d_best_swap) sycl::free(d_best_swap, q);

    if (d_route_pairs) sycl::free(d_route_pairs, q);

    if (d_num_pairs) sycl::free(d_num_pairs, q);
  }

private:
  void setup_route_combs(int minimum_routes, int peak_routes);
};
}