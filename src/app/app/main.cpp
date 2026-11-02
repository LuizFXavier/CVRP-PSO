//TODO: Help

#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <format>

#include <app/command-line.hpp>
#include <libcvrp/engine/io.hpp>
#include <libcvrp/engine/local-search.hpp>
#include <libpso/engine/pso-runner.hpp>
#include <libpso/engine/io/OutputWriter.hpp>
#include <app/RunLogger.hpp>

int 
main(int argc, const char *argv[])
{
  auto [configIO, hyperparameters] = app::parse_cli(argc, argv);

  auto instance = cvrp::io::read_instance(configIO.instance_path);

  instance.build_distance_matrix();

  std::string init_mode;

  if (hyperparameters.fixed_sector){
    init_mode = "fixed";
  }
  else{
    init_mode = "random";
  }

  int swap_cap = hyperparameters.swap_cap;

  auto optimizer = [&](std::vector<std::vector<int>*>& tours, cvrp::Instance& instance, cvrp::PhaseTimes& times) {
    
    for (int i = 0; i < tours.size(); ++i){
      
      auto& mega_tour = *(tours[i]);
      cvrp::local_search::optimize(mega_tour, instance, times, swap_cap);
    }
    
  };

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
    std::format("{}_c1{}_c2{}_sw{}_el{}_it{}_init{}_run{}_seq",
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

    auto best_particle = pso::run_pso(instance, hyperparameters, optimizer, costs, times);

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> execution_time = end_time - start_time;

    std::cout << best_particle.curr_of << "," << execution_time.count() << "\n";

    if (!configIO.output_dir.empty()){
      std::string file_name = instance.name + std::string("_") + std::to_string(i) + std::string(".sol");
    
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
