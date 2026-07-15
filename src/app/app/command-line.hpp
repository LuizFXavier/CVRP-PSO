#pragma once
#include <string>

#include <libpso/core/Hyperparameters.hpp>

namespace app
{
  struct ConfigIO{
    std::string instance_path{};
    std::string output_dir{};
    int runs = 1;
    bool showHelp{};
  };

  struct Parameters{
    ConfigIO configIO;
    pso::Hyperparameters pso;
  };

  Parameters parse_cli(int argc, const char* argv[]);
    
} // namespace app
