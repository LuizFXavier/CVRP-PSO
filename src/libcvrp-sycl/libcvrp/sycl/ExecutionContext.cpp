#include <libcvrp/sycl/ExecutionContext.hpp>

namespace cvrp::sycl_engine
{
void 
ExecutionContext::setup_route_combs(int minimum_routes, int max_routes)
{
  auto d_route_pairs = this->d_route_pairs;

  this->q.submit([&] (sycl::handler& h){ 

    h.single_task([=]() {
      
      int offset = 0;

      for (int i = 0; i < minimum_routes - 1; ++i){
        for (int j = i + 1; j < minimum_routes; ++j){
          d_route_pairs[offset].first = i;
          d_route_pairs[offset].second = j;
          ++offset;
        }
      }
    });
  });

  int minimum_combs = minimum_routes * (minimum_routes - 1) / 2;

  this->q.submit([&] (sycl::handler& h){ 

    h.single_task([=]() {
      
      int offset = minimum_combs;

      for (int i = minimum_routes; i < max_routes; ++i){
        for (int j = 0; j < i; ++j){
          d_route_pairs[offset].first = i;
          d_route_pairs[offset].second = j;
          ++offset;
        }
      }
    });
  });

  this->q.wait();
}

} // namespace cvrp::sycl_engine
