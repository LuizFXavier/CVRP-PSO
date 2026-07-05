//TODO: Executável de fato, que lê da linha de comando e passa a instância para o PSO

//TODO: Help

#include <iostream>

#include <app/command-line.hpp>
#include <libcvrp/engine/io.hpp>
#include <libpso/engine/pso-runner.hpp>

int 
main(int argc, const char *argv[])
{
  auto [configIO, hyperparameters] = app::parse_cli(argc, argv);

  auto instance = cvrp::io::read_instance(configIO.instance_path);

  auto best_particle = pso::run_pso(instance, hyperparameters);

  std::cout << best_particle.curr_of << "\n";

  return 0;
}
