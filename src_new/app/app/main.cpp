//TODO: Help

#include <iostream>
#include <string>

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

  std::string file_name = instance.name + std::string(".sol");

  cvrp::io::save_routes(best_particle.curr_solution, instance, configIO.output_dir, file_name);

  return 0;
}
