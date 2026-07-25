//TODO: Help

#include <iostream>
#include <string>
#include <chrono>

#include <app/command-line.hpp>
#include <libcvrp/engine/io.hpp>
#include <libpso/engine/pso-runner.hpp>

int 
main(int argc, const char *argv[])
{
  auto [configIO, hyperparameters] = app::parse_cli(argc, argv);

  auto instance = cvrp::io::read_instance(configIO.instance_path);

  instance.build_distance_matrix();

  for (int i = 0; i < configIO.runs; ++i){

    auto start_time = std::chrono::high_resolution_clock::now();

    auto best_particle = pso::run_pso(instance, hyperparameters);

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> execution_time = end_time - start_time;

    std::cout << best_particle.curr_of << "," << execution_time.count() << "\n";

    if (!configIO.output_dir.empty()){
      std::string file_name = instance.name + std::string(".sol");
    
      cvrp::io::save_routes(best_particle.curr_solution, instance, configIO.output_dir, file_name);
    }
  }



  return 0;
}
