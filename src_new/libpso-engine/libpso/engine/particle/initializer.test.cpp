#include <stdexcept>
#include <format>

#include <libpso/engine/particle/initializer.hpp>

void random_initializer_test();

int
main(){
  random_initializer_test();

  return 0;
}

void 
random_initializer_test(){

  int dimension = 10;

  std::random_device rd;
  std::mt19937 gen(rd());
  
  auto particle = pso::random_initialize(dimension, gen);

  if (!particle.curr_solution.size()){
    throw std::runtime_error("Error: Random solution not initialized!");
  }
  else if (!particle.curr_solution[0] == 0){
    throw std::runtime_error("Error: Random solution does not start at depot!");
  }
  else if (!particle.curr_solution[dimension] == 0){
    throw std::runtime_error("Error: Random solution does not end at depot!");
  }
  
  std::vector<bool> matches(dimension, false);

  for (int i = 0; i < dimension; ++i){
    matches[particle.curr_solution[i]] = true;
  }

  for (int i = 0; i < dimension; ++i){

    if (matches[i] == false){
      throw std::runtime_error(std::format("Error: Missing client {} on random solution!", i));
    }
  }
}