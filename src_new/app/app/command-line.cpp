#include <stdexcept>
#include <string>
#include <cstring>
#include <format>

#include <app/command-line.hpp>

#include <libpso/core/Hyperparameters.hpp>

namespace app
{

Parameters
parse_cli(int argc, char* argv[])
{
  auto valid_argument = [&](const char* b, int i){
    return std::strcmp(argv[i], b) == 0;
  };

  auto is_help = [&](int i){
    return std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0;
  };

  ConfigIO config{};

  pso::Hyperparameters pso{};

  for(int i = 1; i < argc; ++i){

    std::string arg = argv[i];

    if(is_help(i)){
      config.showHelp = true;

      return {config, pso};
    }

    if (valid_argument("--in", i) || valid_argument("--input", i)){

      if(i+1 > argc)
          throw std::runtime_error(std::format("Error: {} option requires a directory path.", arg));

      config.input_dir = argv[i+1];
      ++i;
    }

    else if(valid_argument("--out", i) || valid_argument("--output", i)){
      if(i+1 > argc)
          throw std::runtime_error(std::format("Error: {} option requires a directory path.", arg));

      config.output_dir = argv[i+1];
      ++i;
    }

    else if(valid_argument("--swarm-size", i) || valid_argument("--swarm", i)){

      try{
        if(i+1 > argc)
          throw std::runtime_error("");

        pso.swarm_size = std::stoi(argv[i+1]);  
      }
      catch(const std::exception& e){
        throw std::runtime_error(std::format("Error: {} option requires an integer value.", arg));
      }
                
      ++i;
    }

    else if(valid_argument("--iterations", i) || valid_argument("--iter", i)){

      try{
        if(i+1 > argc)
          throw std::runtime_error("");

        pso.iterations = std::stoi(argv[i+1]);  
      }
      catch(const std::exception& e){
        throw std::runtime_error(std::format("Error: {} option requires an integer value.", arg));
      }

      ++i;
    }

    else if(valid_argument("--elite", i)){

      try{
        if(i+1 > argc)
          throw std::runtime_error("");

        pso.elite = std::stoi(argv[i+1]);  
      }
      catch(const std::exception& e){
        throw std::runtime_error(std::format("Error: {} option requires an integer value.", arg));
      }

      ++i;
    }

  }

  if(config.input_dir.empty())
    throw std::runtime_error("Error: Missing --input option.");

  if(config.output_dir.empty())
    throw std::runtime_error("Error: Missing --output option.");

  if(pso.iterations <= 0)
    throw std::runtime_error("Error: --iterations setting is missing or has an invalid value.");

  if(pso.swarm_size <= 0)
    throw std::runtime_error("Error: --swarm-size setting is missing or has an invalid value.");

  return {config, pso};
}


} // namespace app
