#pragma once
#include <vector>
#include "Particle.hpp"
#include "Cidade.hpp"

class LocalSearch {
private:
    struct insert_info {
        unsigned id;
        unsigned pred;
        unsigned suce;
        double cost;
    };
public:
    LocalSearch(vector<Cidade>& c, int& capacidadeV):
    cidades(c), capacidadeV(capacidadeV)
    {};
    void melhoria_2_opt(Particle &p);

    void melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des);

    void swap_star(Particle &p);

private:
    vector<insert_info> findTop3Locations(vector<int>& sol, unsigned v, const pair<unsigned,unsigned> &limiters);

    double insertion_cost(unsigned v, unsigned p, unsigned s) {

        return Cidade::distancia(cidades[p], cidades[v]) +
               Cidade::distancia(cidades[v], cidades[s]) -
               Cidade::distancia(cidades[p], cidades[s]);
    }
    double insertion_cost(vector<int>& sol, unsigned id_v, unsigned id_p, unsigned id_s) {

        return insertion_cost(sol[id_v], sol[id_p], sol[id_s]);
    }

    void apply_swap(Particle& p, vector<int>& sol, double cost, int des_v, int des_u, insert_info &v, insert_info &u);
    vector<Cidade>& cidades;
    int& capacidadeV;
};
