#pragma once
#include <vector>
#include <string>
#include <cmath>
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
    int capacidadeV; //Capacidade dos veículos

    Particle best_particle;
    double best_dist = INFINITO;
public:

    int nCidades;
    string instance_name = "";
    vector<Cidade> cidades;
    vector<Particle> particulas;

    int seguir_melhor = 0; //Frequência em que o resultado da melhor partícula é guardado
    int seguir_qualquer = 0; //Frequência em que os resultado de partículas quaisquer são guardados

    int tam_elite = 0;
    
    double calcula_caminho(vector<int> caminho); //Fitness function
    double calcula_caminho(vector<int> caminho, int begin, int end);
    void executar(string routes_file);

    void executar(std::vector<std::vector<Solucao>> &solucoes){
        gerar_particulas();
        main_loop(solucoes);};

    double calcula_distancia(Cidade &a, Cidade &b){ return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));}
    double calcula_distancia(int a, int b){return calcula_distancia(cidades[a], cidades[b]);}
    void set_instance(string config_file);
    
    void set_properties(string config_file);

    void set_nPart(string nParticulas){this->nParticulas = stoi(nParticulas);};

    void set_nRep(string nRep){this->nRep = stoi(nRep);};

    void set_seguir_melhor(string frequencia){this->seguir_melhor = stoi(frequencia);}
    
    void set_seguir_qualquer(string frequencia){this->seguir_qualquer = stoi(frequencia);}

    void set_elite(string n){this->tam_elite = stoi(n);}

    void melhoria_2_opt(Particle &p);

    void melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des);

    vector<int> get_solution(Particle &p);

    inline int getNPart(){return this->nParticulas;}
    inline int getNRep(){return this->nRep;}

    void apresentar(Particle &p);

    Particle& get_best(){return this->best_particle;};
};