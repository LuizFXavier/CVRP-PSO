#pragma once
#include <vector>
#include <utility>
#include <unordered_map>
#include "Velocity.hpp"
#include "Cidade.hpp"
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
    bool operator>(const Particle &p1) const {return this->dist_atual > p1.dist_atual;}
    Velocity operator-(Particle &p1);
    void aplicar_velocidade(Velocity &v);
    vector<int> get_full_solution(vector<Cidade> &cidades, int capacidade);
    double fitness(vector<Cidade> &cidades, int capacidade);

    void atualiza_dist(vector<Cidade> &cidades, int capacidade) {
        this->dist_atual = fitness(cidades, capacidade);
        if (this->dist_atual < this->best_dist) {
            this->best_dist = this->dist_atual;
            this->p_best = this->solucao_atual;
        }
    }
};