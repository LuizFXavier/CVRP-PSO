#include <fstream>
#include <cassert>
#include <cstdio>
#include <string>
#include <stdexcept>

#include <libcvrp/engine/io.hpp>

int
main(){
  
  std::string temp_file = "cvrp_instance_test.txt";

  std::ofstream out(temp_file);
  
  out << "NAME : test\n";
  out << "DIMENSION : 2\n";
  out << "CAPACITY : 100\n";

  out << "NODE_COORD_SECTION\n";
  out << " 1 10 20\n";
  out << " 2 30 40\n";

  out << "DEMAND_SECTION \n";
  out << "1 0\n";
  out << "2 15\n";
  out.close();

  auto instance = cvrp::io::read_instance(temp_file);

  std::remove(temp_file.c_str());

  if (instance.name != std::string("test")){
    throw std::runtime_error("Error: Name not propriately read!");
  }
  if (instance.dimension != 2){
    throw std::runtime_error("Error: Dimension not propriately read!");
  }
  if (instance.capacity != 100){
    throw std::runtime_error("Error: Dimension not propriately read!");
  }
  if (instance.clients.size() != 2){
    throw std::runtime_error("Error: Clients not propriately read!");
  }

  if (instance.clients[0].x != 10){
    throw std::runtime_error("Error: Wrong X coord for depot!");
  }
  if (instance.clients[0].y != 20){
    throw std::runtime_error("Error: Wrong Y coord for depot!");
  }
  if (instance.clients[1].x != 30){
    throw std::runtime_error("Error: Wrong X coord for client!");
  }
  if (instance.clients[1].y != 40){
    throw std::runtime_error("Error: Wrong Y coord for client!");
  }

  if (instance.clients[0].demand != 0){
    throw std::runtime_error("Error: Wrong demand for depot!");
  }  
  if (instance.clients[1].demand != 15){
    throw std::runtime_error("Error: Wrong demand for client!");
  }

  return 0;
}