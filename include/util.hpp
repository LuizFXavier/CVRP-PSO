#pragma once
#include <cmath>
#include <chrono>
#include "Cidade.hpp"
#include "Solucao.hpp"

namespace util
{
    inline void
    guarda_solucao(std::vector<Solucao> &sol, int iteracao, double custo, std::vector<int> rota){

        sol.push_back({iteracao, custo, rota});
    }
    int numero_aleatorio(int n);
    void salva_solucoes(std::vector<Solucao> &solutions, std::string instance_name, int nPart, int nRep, int qual, int set, int elite);
} // namespace util
