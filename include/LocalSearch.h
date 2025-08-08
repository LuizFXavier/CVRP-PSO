#pragma once
#include <vector>
#include "Particle.hpp"
#include "Cidade.hpp"

using namespace std;

class LocalSearch {
private:
    struct insert_info_legacy {
        unsigned id;
        unsigned pred;
        unsigned suce;
        double cost;
    };
    struct insert_info{
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

    void swap_star_legacy(Particle &p);

    void search(Particle &p);

private:
    vector<insert_info_legacy> findTop3Locations(vector<int>& sol, unsigned v, const pair<unsigned,unsigned> &limiters);

    double insertion_cost(unsigned v, unsigned p, unsigned s) {

        return Cidade::distancia(cidades[p], cidades[v]) +
               Cidade::distancia(cidades[v], cidades[s]) -
               Cidade::distancia(cidades[p], cidades[s]);
    }

    double insertion_cost(unsigned v, unsigned p, unsigned s, double cost)
    {
        auto b = insertion_cost(v,p,s);
        auto a = cost - b;
        if (a < 0)
        {
            auto w = Cidade::distancia(cidades[p], cidades[v]);
            auto x = Cidade::distancia(cidades[v], cidades[s]);
            auto y = Cidade::distancia(cidades[p], cidades[s]);
            auto t = w + x - y;
        }

        return a;
    }
    double insertion_cost(vector<int>& sol, unsigned id_v, unsigned id_p, unsigned id_s) {

        return insertion_cost(sol[id_v], sol[id_p], sol[id_s]);
    }

    void two_opt(vector<Route> &r);
    void swap_star(vector<Route> &r);
    void apply_swap(unsigned id_v, unsigned id_u, Route& r, Route& r_ln, insert_info& v_insert, insert_info& u_insert);

    // Custos de inserção de um cliente v em uma rota r'
    vector<insert_info> findTop3Locations(unsigned v, Route& r_ln);

    double route_cost(Route &r)
    {
        double c = 0;
        for (int i = 0; i < r.size() -1; i++)
            c += Cidade::distancia(cidades[r[i]], cidades[r[i+1]]);
        return c;
    }

    void apply_swap(Particle& p, vector<int>& sol, double cost, int des_v, int des_u, insert_info_legacy &v, insert_info_legacy &u);
    vector<Cidade>& cidades;
    int& capacidadeV;
};
