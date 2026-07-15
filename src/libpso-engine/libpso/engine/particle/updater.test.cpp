#include <stdexcept>
#include <vector>
#include <format>

#include <libpso/engine/particle/updater.hpp>

void diff_test();
void sum_test();
void scalar_test();

int
main()
{
  diff_test();
  sum_test();
  scalar_test();

  return 0;
}

void
diff_test()
{

  using namespace pso;

  pso::Particle p1, p2;

  std::vector<int> sol1 = {0,1,2,3,4,5,0};

  std::vector<int> sol2 = {0,1,4,3,2,5,0};

  p1.curr_solution = sol1;
  p2.curr_solution = sol2;

  pso::Velocity v = p1 - p2;

  auto value = v.value[0];

  if (!(value.first == 2 && value.second == 4))
    throw std::runtime_error(std::format("Error: Wrong velocity calculated ({}, {}).", value.first, value.second));
}

void 
sum_test()
{
  pso::Velocity v1, v2;

  v1.value.push_back({1,2});
  v2.value.push_back({3,4});

  auto v3 = v1 + v2;

  if (v3.value.size() < 2){
    
    throw std::runtime_error("Error: Velocity did not perform addition properly! {}");
  }
  if (v3.value[0].first != 1 || v3.value[0].second != 2){
    throw std::runtime_error("Error: First term of velocity addition is incorrect!");
  }
  if (v3.value[1].first != 3 || v3.value[1].second != 4){
    throw std::runtime_error("Error: Second term of velocity addition is incorrect!");
  }
}

void
scalar_test(){

  pso::Velocity v1;

  v1.value.push_back({1,2});
  v1.value.push_back({3,4});

  pso::Velocity v_half = v1 * 0.5;

  if (v_half.value.size() != 1){  
    throw std::runtime_error("Error: Wrong velocity multiplication by 0.5!");
  }
  if (v_half.value[0].first != 1 || v_half.value[0].second != 2){  
    throw std::runtime_error("Error: Wrong values on velocity multiplication by 0.5!");
  }

  pso::Velocity v_plus_half = v1 * 1.5;

  if (v_plus_half.value.size() != 3){  
    throw std::runtime_error("Error: Wrong velocity multiplication by 1.5!");
  }

  if (v_plus_half.value[0].first != 1 || v_plus_half.value[0].second != 2 ||
      v_plus_half.value[1].first != 3 || v_plus_half.value[1].second != 4 ||
      v_plus_half.value[2].first != 1 || v_plus_half.value[2].second != 2){  
    throw std::runtime_error("Error: Wrong values on velocity multiplication by 1.5!");
  }
  
  pso::Velocity v_double = v1 * 2;

  if (v_double.value.size() != 4){  
    throw std::runtime_error("Error: Wrong velocity multiplication by 2!");
  }
  
  if (v_double.value[0].first != 1 || v_double.value[0].second != 2 ||
      v_double.value[1].first != 3 || v_double.value[1].second != 4 ||
      v_double.value[2].first != 1 || v_double.value[2].second != 2 ||
      v_double.value[3].first != 3 || v_double.value[3].second != 4){  
    throw std::runtime_error("Error: Wrong values on velocity multiplication by 1.5!");
  }
}