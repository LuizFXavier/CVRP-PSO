#pragma once
#include <string>
#include <stdexcept>
#include <iostream>
#include "PSO.hpp"
class CommandLine
{
public:
    CommandLine(int argc, char* argv[], PSO &pso);
};

CommandLine::CommandLine(int argc, char* argv[], PSO &pso)
{
    if(argc < 5){
        throw std::runtime_error("Número errado de argumentos!");
    }

    for(int i = 1; i < argc; i++){
        if(std::string(argv[i]) == "-instancia" && i+1 < argc){
            pso.set_instance(argv[i+1]);
        }
        else if(std::string(argv[i]) == "-config" && i+1 < argc){
            pso.set_properties(argv[i+1]);
        }
        else if(std::string(argv[i]) == "-npart" && i+1 < argc){
            pso.set_nPart(argv[i+1]);
        }
        else if(std::string(argv[i]) == "-nrep" && i+1 < argc){
            pso.set_nRep(argv[i+1]);
        }
        else if(std::string(argv[i]) == "-seguir-melhor" && i+1 < argc){
            pso.set_seguir_melhor(argv[i+1]);
        }

    }
}