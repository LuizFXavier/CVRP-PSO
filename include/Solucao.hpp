#pragma once
#include <vector>
#include "Particle.hpp"

namespace solucao{

    struct Solucao{
        int iteracao;
        double custo;
        std::vector<int> rota;
    };

    extern std::vector<Solucao> melhor_solucao;

    inline void
    guarda_solucao(int iteracao, double custo, std::vector<int> rota){

        melhor_solucao.push_back({iteracao, custo, rota});
    }

    void salva_solucoes(std::string instance_name, int nPart, int nRep);
}
