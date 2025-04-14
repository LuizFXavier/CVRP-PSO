#pragma once
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include "PSO.hpp"
#include "Solucao.hpp"
class CommandLine
{
public:
    CommandLine(int argc, char* argv[], PSO &pso, std::vector<std::vector<Solucao>> &solucoes);
};

CommandLine::CommandLine(int argc, char* argv[], PSO &pso, std::vector<std::vector<Solucao>> &solucoes)
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
        else if(std::string(argv[i]) == "-seguir-qualquer" && i+2 < argc){
            pso.set_seguir_qualquer(argv[i+1]);
            solucoes.resize(stoi(argv[i+2]) + 1);
        }

    }
}