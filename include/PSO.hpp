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
    
    int nParticulas = 10;
    double c1 = 1;  //Coeficiente cognitivo
    double c2 = 1;  //Coeficiente social
    double w_min = 0.2; //Coeficiente de inércia mínimo
    double w_max = 1; //Coeficiente de inércia máximo
    int nRep = 10;  //Número de iterações a serem performadas
    int capacidadeV = 10;

    Particle best_particle;
    double best_dist = INFINITO;

    vector<std::vector<double>> distancias;

public:
    vector<Cidade> cidades;
    double calcula_caminho(vector<int> caminho); //Fitness function
    int nCidades;
    vector<Particle> particulas;
    PSO(string cities_file);
    void executar(string routes_file);
    void executar();
    void set_properties(string config_file);

    vector<int> get_solution(Particle p);
    void apresentar(Particle &p);

    Particle get_best();
};