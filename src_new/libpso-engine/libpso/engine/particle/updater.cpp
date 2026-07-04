#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <format>

#include <libpso/engine/particle/updater.hpp>

#include <libcvrp/core/Client.hpp>

namespace pso
{
Velocity 
operator-(const Particle& p1, const Particle& p2)
{
  Velocity v;

  std::unordered_map<int, int> positions;

  std::vector<int> updated_path = p2.curr_solution;

  for(int i = 0; i < p2.curr_solution.size() -1; i++){
      
    positions[p1.curr_solution[i]] = i;
  }

  // Percorre a solução alvo
  for(int i = 0; i < p2.curr_solution.size(); i++){
      
    int current_match = p1.curr_solution[i];

    // Enquanto o valor naquela posição não for o correto, vai colocando o valor encontrado na posição correta
    while(current_match != updated_path[i]){

      int k = positions[updated_path[i]];

      v.value.push_back(std::pair(i, k));

      int temp = updated_path[i];
      updated_path[i] = updated_path[k];
      updated_path[k] = temp;

      if(i == 0)
        updated_path[updated_path.size() -1] = updated_path[0];
        
    }
  }

  return v;
}

Velocity 
operator+(const Velocity& v1, const Velocity& v2) 
{
  Velocity result;

  result.value = v1.value;

  for (const auto& val : v2.value) {
    result.value.push_back(val);
  }
  return result;
}

Velocity 
operator*(const Velocity& v, float c) 
{
  Velocity result;

  if (c == 0.0f) 
    return result;

  int t = static_cast<int>(c);
  float k = c - t;

  for (int i = 0; i < t; i++) {
    for (const auto& val : v.value) {
      result.value.push_back(val);
    }
  }

  // Adiciona a parte fracionária
  int k_lim = static_cast<int>(k * v.value.size());

  for (int i = 0; i < k_lim; i++) {
    result.value.push_back(v.value[i]);
  }

  return result;
}

void 
apply_velocity(Particle &p, Velocity& v)
{
  for(int i = 0; i < v.value.size(); i++){
        
    int aux = p.curr_solution[v.value[i].first];

    p.curr_solution[v.value[i].first] = p.curr_solution[v.value[i].second];
    p.curr_solution[v.value[i].second] = aux;
    
    if(v.value[i].first == 0){
        p.curr_solution[p.curr_solution.size() -1] = p.curr_solution[v.value[i].first];
    }
    else if(v.value[i].second == 0){
        p.curr_solution[p.curr_solution.size() -1] = p.curr_solution[v.value[i].second];
    }
  }
}

float 
fitness(Particle& p, cvrp::Instance& instance)
{

  float distance = 0;
  int curr_capacity = instance.capacity;

  auto& clients = instance.clients;

  for(int i = 0; i < instance.dimension; i++){

    if(clients[p.curr_solution[i+1]].demand <= curr_capacity){

      curr_capacity -= clients[p.curr_solution[i + 1]].demand;
      distance += cvrp::distance(clients[p.curr_solution[i]], clients[p.curr_solution[i+1]]);
    }
    else{
      distance += cvrp::distance(clients[p.curr_solution[i]], clients[0]);
      curr_capacity = instance.capacity;
      distance += cvrp::distance(clients[0], clients[p.curr_solution[i+1]]);
      curr_capacity -= clients[p.curr_solution[i+1]].demand;
    }

  }
  return distance;
}
} // namespace pso