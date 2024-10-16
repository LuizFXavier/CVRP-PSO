#pragma once
#include <vector>
#include <string>
#include "Cidade.hpp"
#include "Particle.hpp"
#define INFINITO 0xfffffff

using namespace std;
class PSO
{
private:
    double calcula_distancia(Cidade &a, Cidade &b);
    void main_loop();
    void gerar_particulas();
    

    double c1 = 1;  //Coeficiente cognitivo
    double c2 = 1;  //Coeficiente social
    double w_max = 1; //Coeficiente de inércia máximo
    double w_min = 0.2; //Coeficiente de inércia mínimo
    int nRep = 10;  //Número de iterações a serem performadas
    int capacidadeV = 10;

    Particle best_particle;
    double best_dist = INFINITO;

    vector<std::vector<double>> distancias;

public:
    vector<Cidade> cidades;
    double calcula_caminho(vector<int> caminho); //Fitness function
    int nCidades;
    int nParticulas = 10;
    vector<Particle> particulas;
    PSO(string cities_file);
    void executar(string routes_file);
    void executar();

    vector<int> get_solution(Particle p);
    void apresentar(Particle &p);

    Particle get_best();
};