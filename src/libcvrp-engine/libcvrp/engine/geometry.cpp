#include <vector>

#include <libcvrp/engine/geometry.hpp>

namespace cvrp
{
  std::vector<float>
  compute_distances(std::vector<Client> &clients)
  {
    std::vector<float> distances(clients.size() * clients.size());

    for (int i = 0; i < clients.size(); ++i){
      for (int j = 0; j < clients.size(); ++j){
        distances[i * clients.size() + j] = distance(clients[i], clients[j]);
      }
    }
    return distances;
  }

} // namespace cvrp
