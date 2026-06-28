#include <vector>
#include <string>
#include <stdexcept>

#include <app/command-line.hpp>

using namespace std;

bool simple_success();
bool missing_input();
bool missing_output();
bool missing_input_value();
bool missing_output_value();
bool help_detected();

bool missing_swarm_size_value();
bool missing_iterations_value();
bool invalid_swarm_size_value();
bool invalid_iterations_value();

int main(){
  if(!simple_success()){
    throw runtime_error("Error: Arguments not detected!");
  }
  if(!help_detected()){
    throw runtime_error("Error: Help not detected!");
  }
  if(!missing_input()){
    throw runtime_error("Error: Missing input not detected!");
  }
  if(!missing_output()){
    throw runtime_error("Error: Missing output not detected!");
  }
  if(!missing_input_value()){
    throw runtime_error("Error: Invalid input not detected!");
  }
  if(!missing_output_value()){
    throw runtime_error("Error: Invalid output not detected!");
  }
  if(!missing_iterations_value()){
    throw runtime_error("Error: Missing iterations value not detected!");
  }
  if(!missing_swarm_size_value()){
    throw runtime_error("Error: Missing swarm size value not detected!");
  }
  if(!invalid_iterations_value()){
    throw runtime_error("Error: Invalid iterations value not detected!");
  }
  if(!invalid_swarm_size_value()){
    throw runtime_error("Error: Invalid swarm size value not detected!");
  }

  return 0;
}

bool
simple_success(){
  vector<const char*> argv = {"test", 
                              "--input", "in_dir", 
                              "--output", "out_dir", 
                              "--iter", "20",
                              "--swarm", "20"};

  auto [config, pso] = app::parse_cli(argv.size(), (char**)argv.data());
  
  return config.input_dir == "in_dir" && config.output_dir == "out_dir" &&
         pso.swarm_size == 20 && pso.iterations == 20;
}


bool
help_detected(){
  vector<const char*> argv = {"test", "--help"};

  auto [config, _] = app::parse_cli(argv.size(), (char**)argv.data());
  
  return config.showHelp;
}

bool
missing_input(){

  vector<const char*> argv = {"test", "--output", "out_dir"};

  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
missing_output(){

  vector<const char*> argv = {"test", "--input", "in_dir"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
missing_input_value(){

  vector<const char*> argv = {"test", "--in"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
missing_output_value(){

  vector<const char*> argv = {"test", "--out"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
missing_iterations_value(){

  vector<const char*> argv = {"test", "--iter"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
missing_swarm_size_value(){

  vector<const char*> argv = {"test", "--swarm-size"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
invalid_iterations_value(){

  vector<const char*> argv = {"test", "--iter", "-2"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}

bool
invalid_swarm_size_value(){

  vector<const char*> argv = {"test", "--swarm-size", "0"};
  
  try {
    auto parameters = app::parse_cli(argv.size(), (char**)argv.data());
  } catch (const std::exception& e) {
    return true;
  }
  return false;
}