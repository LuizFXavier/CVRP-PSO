#pragma once
#include <vector>
#include "Particle.hpp"
#include "Cidade.hpp"

class LocalSearch {
    public:
    LocalSearch(vector<Cidade>& c, int& capacidadeV):
    cidades(c), capacidadeV(capacidadeV)
    {};
    void melhoria_2_opt(Particle &p);

    void melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des);

private:
    vector<Cidade>& cidades;
    int& capacidadeV;
};
