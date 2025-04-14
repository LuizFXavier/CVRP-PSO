#pragma once
#include <cmath>
#include <chrono>
#include "Cidade.hpp"
#include "Solucao.hpp"

namespace util
{
    inline double
    calcula_distancia(Cidade &a, Cidade &b){ 
        return sqrt(pow(a.x - b.x, 2) + pow(a.y - a.y, 2));
    }

    // unsigned time_seed = std::chrono::system_clock::now().time_since_epoch().count();

    inline void
    guarda_solucao(std::vector<Solucao> &sol, int iteracao, double custo, std::vector<int> rota){

        sol.push_back({iteracao, custo, rota});
    }
    void salva_solucoes(std::vector<Solucao> &solutions, std::string instance_name, int nPart, int nRep, int qual);
} // namespace util
