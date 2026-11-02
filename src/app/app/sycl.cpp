#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

#include <app/command-line.hpp>
#include <libcvrp/engine/io.hpp>
#include <libcvrp/sycl/ExecutionContext.hpp> 
#include <libcvrp/sycl/local-search.hpp>
#include <libpso/engine/pso-runner.hpp>
#include <libpso/engine/io/OutputWriter.hpp>
#include <app/RunLogger.hpp>

int 
main(int argc, const char *argv[])
{
  auto [configIO, hyperparameters] = app::parse_cli(argc, argv);
  auto instance = cvrp::io::read_instance(configIO.instance_path);
  
  instance.build_distance_matrix();

  int simultaneous_particles = hyperparameters.swarm_size; 
  cvrp::sycl_engine::ExecutionContext ctx;
  ctx.setup(instance, hyperparameters.elite_size);

  int swap_cap = hyperparameters.swap_cap;

  // Cria a função injetável usando lambda tendo acesso às variáveis 'instance' e 'ctx' da main
  auto sycl_optimizer = [&](std::vector<std::vector<int>*>& tours, cvrp::Instance& instance, cvrp::PhaseTimes& times) {

    cvrp::sycl_engine::local_search::optimize(tours, instance, ctx, times, swap_cap);
  };

  std::string init_mode;

  if (hyperparameters.fixed_sector){
    init_mode = "fixed";
  }
  else{
    init_mode = "random";
  }

  int run_id = configIO.start_run;
  int remaining_runs = configIO.runs - (configIO.start_run - 1);

  if (configIO.save_detail)
    remaining_runs += 1;

  pso::io::OutputWriter* ow = nullptr;

  if (configIO.save_detail)
    ow = new pso::io::OutputWriter(configIO.detail_output_file);


  for (int i = 0; i < remaining_runs; ++i){

    pso::PartialCost costs{};
    pso::PhaseTimes times{};

    auto base = fs::path(configIO.output_dir) /
    std::format("{}_c1{}_c2{}_sw{}_el{}_it{}_init{}_run{}_sycl",
              instance.name, hyperparameters.cognitive_c, hyperparameters.social_c,
              hyperparameters.swarm_size, hyperparameters.elite_size, hyperparameters.iterations, init_mode, i);

  
    RunLogger logger(base.string(), hyperparameters.bks);

    pso::Observers obs;
    obs.stats_every = 10;
    obs.on_init = [&](const pso::InitInfo& s) {
      logger.log_init(s.swarm_size, s.best_cost, s.mean_cost, s.distinct);
    };
    obs.on_iteration = [&](const pso::IterationInfo& it) {
      logger.log_iteration(it.iter, it.g_best_of, it.swarm.distinct, it.swarm.frac_eq_gbest, times.optimizer);
    };

    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto best_particle = pso::run_pso(instance, hyperparameters, sycl_optimizer, costs, times, &obs);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> execution_time = end_time - start_time;

    std::cout << best_particle.curr_of << "," << execution_time.count() << "\n";

    if (!configIO.output_dir.empty()){
      std::string file_name = instance.name + std::string(".sol");
      cvrp::io::save_routes(best_particle.curr_solution, instance, configIO.output_dir, file_name);
    }

    if (configIO.save_detail && i != 0){

      ow->save_data(instance.name, run_id, costs, times, execution_time.count(), hyperparameters);

      ++run_id;
    }
  }

  delete ow;

  return 0;
}
