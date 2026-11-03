#include <filesystem>
#include <stdexcept>

#include <libpso/engine/io/OutputWriter.hpp>
#include <libpso/core/Hyperparameters.hpp>

namespace pso::io
{
OutputWriter::OutputWriter(std::filesystem::path path)
{
  this->file_path = path;

  if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
    std::filesystem::create_directories(path.parent_path());
  }

  file.open(path, std::ios::out | std::ios::app);

  if (!file.is_open()) {
    throw std::runtime_error("Erro: Não foi possível abrir o arquivo de saída em: " + path.string());
  }
}

OutputWriter::~OutputWriter()
{
  if(file.is_open())
    file.close();
}

void 
pso::io::OutputWriter::save_data(std::string instance_name, 
                                 int run_id, 
                                 PartialCost costs, 
                                 PhaseTimes times,
                                 double runtime, 
                                 Hyperparameters hyperparameters)
{

  if (std::filesystem::exists(this->file_path) && std::filesystem::file_size(this->file_path) == 0){

    std::string header{};

    header += std::string("instance-name,run-id,iterations,swarm-size,elite-size,sectorized,w-max,w-min,cognitive,social,fixed-sector,");

    header += std::string("initial-cost,1/4-cost,1/2-cost,3/4-cost,cost,");

    header += std::string("fitness-time,velocity-time,position-time,import-time,two-opt-time,swap-star-time,runtime\n");

    file << header;
  }

  std::string payload{};

  payload += instance_name + std::string(",");

  payload += std::to_string(run_id) + std::string(",");

  payload += std::to_string(hyperparameters.iterations) + std::string(",");
  payload += std::to_string(hyperparameters.swarm_size) + std::string(",");
  payload += std::to_string(hyperparameters.elite_size) + std::string(",");
  payload += std::to_string(hyperparameters.sectorized) + std::string(",");
  payload += std::to_string(hyperparameters.w_max) + std::string(",");
  payload += std::to_string(hyperparameters.w_min) + std::string(",");
  payload += std::to_string(hyperparameters.cognitive_c) + std::string(",");
  payload += std::to_string(hyperparameters.social_c) + std::string(",");
  payload += std::to_string(int(hyperparameters.fixed_sector)) + std::string(",");


  payload += std::to_string(costs.initial) + std::string(",");
  payload += std::to_string(costs.quarter) + std::string(",");
  payload += std::to_string(costs.half) + std::string(",");
  payload += std::to_string(costs.three_fourths) + std::string(",");
  payload += std::to_string(costs.final) + std::string(",");

  payload += std::to_string(times.fitness_calculation) + std::string(",");
  payload += std::to_string(times.velocity_update) + std::string(",");
  payload += std::to_string(times.position_update) + std::string(",");
  payload += std::to_string(times.optimizer.import) + std::string(",");
  payload += std::to_string(times.optimizer.two_opt) + std::string(",");
  payload += std::to_string(times.optimizer.swap_star) + std::string(",");
  payload += std::to_string(runtime);

  this->file << payload << std::endl;
}
} // namespace pso::io
