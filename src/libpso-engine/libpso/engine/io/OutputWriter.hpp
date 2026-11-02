#pragma once

#include <string>
#include <filesystem>
#include <fstream>

#include <libpso/core/PartialCost.hpp>
#include <libpso/core/PhaseTimes.hpp>
#include <libpso/core/Hyperparameters.hpp>

namespace pso::io
{

class OutputWriter
{
private:

public:
std::filesystem::path file_path;
std::ofstream file;
    OutputWriter(std::filesystem::path path);

    void save_data(std::string instance_name, int run_id, PartialCost costs, PhaseTimes times, double runtime, Hyperparameters hyperparameters);

    ~OutputWriter();
};

} // namespace pso::io