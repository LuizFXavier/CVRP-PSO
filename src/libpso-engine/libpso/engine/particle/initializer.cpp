#include <vector>
#include <algorithm>
#include <chrono>
#include <random>

#include <libpso/engine/particle/initializer.hpp>

namespace pso
{
  
Particle 
random_initialize(int dimension, std::mt19937& gen)
{
  Particle particle;

  particle.curr_solution.resize(dimension + 1);

  for(int i = 0; i < dimension; i++){
    particle.curr_solution[i] = i;
  }

  particle.curr_solution[dimension] = particle.curr_solution[0];

    
  std::shuffle(particle.curr_solution.begin() + 1, particle.curr_solution.end() - 1, gen);
  
  return particle;
}

// namespace anônimo para funções do sector_initialize
namespace
{
  int 
  demand_sum(cvrp::Instance& instance, std::vector<int>& indexes, int begin, int end)
  {
      
    int d = 0;
    
    for(int i = begin; i < end; i++){
      d += instance.clients[indexes[i]].demand;
    }

    return d;
  }

  int 
  best_r(cvrp::Instance& instance, std::vector<int>& indexes, int begin)
  {
      
    int best_r = begin;
    int left = begin;
    int right = indexes.size();

    while (left <= right)
    {   
        
      int middle = (left + right) / 2;
      int total_demand = demand_sum(instance, indexes, begin, right);
      
      if (total_demand == 0){
        break;
      }

      if(total_demand <= instance.capacity){
          
        best_r = middle;
        left = middle + 1;
      }
      else{
        right = middle - 1;
      }
        
    }
    
    return best_r;
  }
}


Particle
sector_initialize(cvrp::Instance& instance, std::mt19937& gen)
{
  auto& clients = instance.clients;

  std::vector<int> indexes(instance.dimension -1);
    
  for(int i = 0; i < instance.dimension -1; i++){
    indexes[i] = i+1;
  }

  std::uniform_int_distribution<> distrib(1, instance.dimension - 1);

  int p_sel = distrib(gen);

  auto squareDistanceCalc = [&](int id) {
    double dx = clients[id].x - clients[p_sel].x;
    double dy = clients[id].y - clients[p_sel].y;
    return dx*dx + dy*dy;
  };
  
  auto cmp = [&](int a, int b) {
    return squareDistanceCalc(a) < squareDistanceCalc(b);
  };

  std::vector<int> route = {0};
  
  int left = 0;
  int right = 0;
  int a = 0;

  do
  {
    std::sort(indexes.begin() + left, indexes.end(), cmp);

    right = best_r(instance, indexes, left);
    
    right = right == left ? right + 1 : right;

    for(int i = left; i < right; i++)
      route.push_back(indexes[i]);

    left = right;
    
    p_sel = left < indexes.size() ? indexes[right] : distrib(gen);
      
  }
  while (right < indexes.size());
    
  route.push_back(0);

  Particle particle;

  particle.curr_solution = std::move(route);
  
  return particle;
}

} // namespace pso
