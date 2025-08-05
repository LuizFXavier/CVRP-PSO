#pragma once
#include <vector>
#include "Particle.hpp"

struct Solucao{
    
    int iteracao;
    double custo;
    std::vector<int> rota;
};
