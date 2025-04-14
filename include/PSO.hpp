#pragma once
#include <vector>
#include <string>
#include "Cidade.hpp"
#include "Particle.hpp"
#include "Solucao.hpp"
#define INFINITO 0xfffffff

using namespace std;
class PSO
{
private:
    void main_loop(std::vector<std::vector<Solucao>> &solucoes);
    void gerar_particulas();

    int deposito; //id da localidade do depósito na lista de cidades

    int nParticulas = 10;
    double c1 = 1;  //Coeficiente cognitivo
    double c2 = 1;  //Coeficiente social
    double w_min = 0.1; //Coeficiente de inércia mínimo
    double w_max = 1; //Coeficiente de inércia máximo
    int nRep = 10;  //Número de iterações a serem performadas
    int capacidadeV;

    Particle best_particle;
    double best_dist = INFINITO;

    void set_cities();
public:

    string instance_name = "";
    vector<Cidade> cidades;
    int nCidades;
    vector<Particle> particulas;

    int seguir_melhor = 0;
    int seguir_qualquer = 0;
    
    double calcula_caminho(vector<int> caminho); //Fitness function
    void executar(string routes_file);
    void executar(std::vector<std::vector<Solucao>> &solucoes);

    void set_instance(string config_file);
    
    void set_properties(string config_file);

    void set_nPart(string nParticulas);

    void set_nRep(string nRep);

    inline void set_seguir_melhor(string frequencia){this->seguir_melhor = stoi(frequencia);}
    
    inline void set_seguir_qualquer(string frequencia){this->seguir_qualquer = stoi(frequencia);}

    vector<int> get_solution(Particle &p);

    inline int getNPart(){return this->nParticulas;}
    inline int getNRep(){return this->nRep;}

    void apresentar(Particle &p);

    Particle get_best();
};