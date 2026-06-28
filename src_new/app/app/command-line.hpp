#pragma once
#include <string>

#include <libpso/core/Hyperparameters.hpp>

namespace app
{
  struct ConfigIO{
    std::string input_dir;
    std::string output_dir;
    bool showHelp;
  };

  struct Parameters{
    ConfigIO configIO;
    pso::Hyperparameters pso;
  };

  Parameters parse_cli(int argc, char* argv[]);
    
} // namespace app
