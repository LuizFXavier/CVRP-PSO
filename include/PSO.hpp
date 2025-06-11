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
    void gerar_particulas_aleatorias();
    void gerar_particulas_setorizadas();

    Particle best_particle;
    double best_dist = INFINITO;
    
    double c1 = 1;  //Coeficiente cognitivo
    double c2 = 1;  //Coeficiente social
    double w_min = 0.1; //Coeficiente de inércia mínimo
    double w_max = 1; //Coeficiente de inércia máximo
    int nRep = 10;  //Número de iterações a serem performadas
    int nParticulas = 10;
    int capacidadeV; //Capacidade dos veículos
    int deposito; //id da localidade do depósito na lista de cidades

    public:
    
    int nCidades;
    string instance_name = "";
    vector<Cidade> cidades;
    vector<Particle> particulas;
    
    int setorizar = 0; // Quantidade de partículas que passam pela setorização
    int seguir_melhor = 0; //Frequência em que o resultado da melhor partícula é guardado
    int seguir_qualquer = 0; //Frequência em que os resultado de partículas quaisquer são guardados

    int tam_elite = 0;
    
    double calcula_caminho(vector<int> caminho); //Fitness function
    double calcula_caminho(vector<int> caminho, int begin, int end);
    void executar(string routes_file);

    void executar(std::vector<std::vector<Solucao>> &solucoes);

    double calcula_distancia(Cidade &a, Cidade &b){ return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));}
    double calcula_distancia(int a, int b){return calcula_distancia(cidades[a], cidades[b]);}
    void set_instance(string config_file);
    
    void set_properties(string config_file);

    void set_nPart(string nParticulas){this->nParticulas = stoi(nParticulas);};

    void set_nRep(string nRep){this->nRep = stoi(nRep);};

    void set_seguir_melhor(string frequencia){this->seguir_melhor = stoi(frequencia);}
    
    void set_seguir_qualquer(string frequencia){this->seguir_qualquer = stoi(frequencia);}

    void set_elite(string n){this->tam_elite = stoi(n);}

    void set_setorizar(string s){this->setorizar = stoi(s);};

    void melhoria_2_opt(Particle &p);

    void melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des);

    int melhor_r(vector<int> &rota, int begin);

    int soma_demanda(vector<int> &rota, int begin, int end);

    vector<int> get_solution(Particle &p);

    inline int getNPart(){return this->nParticulas;}
    inline int getNRep(){return this->nRep;}

    void apresentar(Particle &p);

    Particle& get_best(){return this->best_particle;};
};

void inline
PSO::gerar_particulas(){
    if(this->setorizar)
        gerar_particulas_setorizadas();
    
    if(this->setorizar < this->nParticulas)
        gerar_particulas_aleatorias();
}

void inline
PSO::executar(std::vector<std::vector<Solucao>> &solucoes){
    gerar_particulas();
    main_loop(solucoes);
}