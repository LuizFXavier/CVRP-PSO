#pragma once
#include <vector>
#include <utility>
#include <unordered_map>
#include "Velocity.hpp"
#define INFINITO 0xfffffff

using namespace std;
class Particle
{
public:
    std::vector<int> p_best;
    double best_dist = INFINITO;
    std::vector<int> solucao_atual;
    double dist_atual;
    Velocity velocity;

    Particle(){};
    Particle(vector<int> solution):solucao_atual(solution){};
    bool operator<(const Particle &p1) const {return this->dist_atual < p1.dist_atual;}
    Velocity operator-(Particle &p1);
    void aplicar_velocidade(Velocity &v);

};