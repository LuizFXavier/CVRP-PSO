#include <iostream>
#include <chrono>
#include <string>
#include <functional>

#include <app/command-line.hpp>
#include <libcvrp/engine/io.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp> 
#include <libcvrp/sycl/local-search.hpp>
#include <libpso/engine/pso-runner.hpp>

int 
main(int argc, const char *argv[])
{
  auto [configIO, hyperparameters] = app::parse_cli(argc, argv);
  auto instance = cvrp::io::read_instance(configIO.instance_path);
  
  instance.build_distance_matrix();

  int simultaneous_particles = hyperparameters.swarm_size; 
  cvrp::sycl_engine::ExecutionContext ctx(simultaneous_particles, instance.clients.size());
  ctx.load_instance(instance);

  // Cria a função injetável usando lambda tendo acesso às variáveis 'instance' e 'ctx' da main
  auto sycl_optimizer = [&](std::vector<int>& tour, cvrp::Instance& instance, int particle_id) {
      cvrp::sycl_engine::local_search::optimize(tour, instance, particle_id, ctx);
  };

  for (int i = 0; i < configIO.runs; ++i){

    auto start_time = std::chrono::high_resolution_clock::now();

    
    auto best_particle = pso::run_pso(instance, hyperparameters, sycl_optimizer);

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