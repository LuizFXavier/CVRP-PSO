#include <fstream>
#include <string>
#include <regex>

#include <libcvrp/engine/io/instance-reader.hpp>

namespace cvrp::io
{

Instance
read_instance(std::string instance_path)
{
  std::ifstream instance_file(instance_path);

  std::string line;
  
  std::smatch match;
  std::string key;

  std::regex word_regex("[\\w-]+");
  std::regex number_regex("[0-9]*\\.?[0-9]+");
  
  int a = 0;

  Instance instance;

  do{
    getline(instance_file, line);
    
    regex_search(line, match, word_regex);
    
    key = match.str();

    if(key == "NAME"){

      line = match.suffix();
      regex_search(line, match, word_regex);
      instance.name = match.str();

    }
    else if (key == "DIMENSION"){
      
      regex_search(line, match, number_regex);
      instance.dimension = stoi(match.str());

    }
    else if (key == "CAPACITY"){
        regex_search(line, match, number_regex);
        instance.capacity = stoi(match.str());
    }

  } 
  while (key != "NODE_COORD_SECTION");
    
  { //Leitura das coordenadas dos clientes
    float x, y;
    int demand;
    int index;
    
    for(int i = 0; i < instance.dimension; ++i){
      
      // Lê o índice do cliente no arquivo
      instance_file >> index;

      instance_file >> x;
      instance_file >> y;

      instance.clients.push_back(Client{x, y});
    }
    
    //Início da seção de demandas
    instance_file >> line;
    
    for(int i = 0; i < instance.dimension; ++i){
      
      // Lê o índice do cliente no arquivo
      instance_file >> index;
      instance_file >> demand;
        
      instance.clients[i].demand = demand;
    }
  }
    
  instance_file.close();
  
  return instance;
    
}


} // namespace cvrp::io
