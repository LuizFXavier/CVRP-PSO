#include <fstream>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <algorithm>
#include <random>
#include <stdexcept>
#include "Cidade.hpp"
#include "PSO.hpp"

PSO::PSO(string cities_file)
{
    srand(time(0)); //Seed para geração de números aleatórios posteriormente.
    ifstream c_file(cities_file);

    c_file >> nCidades;

    { //Leitura das cidades
        double x, y;
        int demanda;
        for(int i = 0; i < nCidades; i++){
            c_file >> x;
            c_file >> y;
            c_file >> demanda;
            this->cidades.push_back(Cidade(x, y, demanda));
        }
    }

    distancias.resize(nCidades);
    
    for (int i = 0; i < nCidades; i++){ //Cálculo das distâncias entre as cidades

        vector <double> c(nCidades);
        
        for (int j = 0; j < nCidades; j++){

            if (j > i)
                c[j] = calcula_distancia(cidades[i], cidades[j]);
            else if (j < i)
                c[j] = distancias[j][i];
            else
                c[j] = 0;
            
        }
        distancias[i] = c;
    }
    this->best_particle.best_dist = INFINITO;
}

void PSO::executar(string routes_file)
{
    ifstream r_file(routes_file);
    r_file >> nParticulas;
    
    for(int i = 0; i < nParticulas; i++){
        vector<int> rota(nCidades +1);
        
        for(int j = 0; j <= nCidades; j++){
            r_file >> rota[j];
        }
        
        this->particulas.push_back(Particle(rota));

    }
    for(int i = 0; i < nParticulas; i++){
        for(int j = 0; j <= nCidades; j++){
            
            cout << particulas[i].solucao_atual[j] << " ";
        }
        cout << endl << endl;
    }

    main_loop();

}


void PSO::executar()
{
    gerar_particulas();
    main_loop();
}
vector<int> PSO::get_solution(Particle p)
{
    double distancia = 0;
    vector<int> sol;
    int capcAtual = this->capacidadeV;
    sol.push_back(0);

    for(int i = 0; i < nCidades; i++){
        
        if(this->cidades[p.solucao_atual[i+1]].demanda <= capcAtual){
            
            capcAtual -= this->cidades[p.solucao_atual[i + 1]].demanda;

            distancia += distancias[p.solucao_atual[i]][p.solucao_atual[i+1]];

            sol.push_back(p.solucao_atual[i+1]);
        }
        else{
            distancia += distancias[p.solucao_atual[i]][0];
            sol.push_back(0);
            capcAtual = this->capacidadeV;

            distancia += distancias[0][p.solucao_atual[i+1]];
            sol.push_back(p.solucao_atual[i+1]);
            capcAtual -= this->cidades[p.solucao_atual[i+1]].demanda;
        }
        
    }
    return sol;
}
void PSO::apresentar(Particle &p)
{
    double distancia = 0;
    int capcAtual = this->capacidadeV;
    cout << "0 ";

    for(int i = 0; i < nCidades; i++){
        
        if(this->cidades[p.solucao_atual[i+1]].demanda <= capcAtual){
            
            capcAtual -= this->cidades[p.solucao_atual[i + 1]].demanda;

            distancia += distancias[p.solucao_atual[i]][p.solucao_atual[i+1]];

            cout << p.solucao_atual[i+1] << " ";
        }
        else{
            distancia += distancias[p.solucao_atual[i]][0];
            cout << "0 ";
            capcAtual = this->capacidadeV;

            distancia += distancias[0][p.solucao_atual[i+1]];
            cout << p.solucao_atual[i+1] << " ";
            capcAtual -= this->cidades[p.solucao_atual[i+1]].demanda;
        }
        
    }
    cout << ": " << distancia << "\n";
}



int random_number(int i) { return rand() % i; }

void PSO::gerar_particulas()
{
    vector<int> rota(nCidades + 1);
    for(int i = 0; i < nCidades; i++){
        rota[i] = i;
    }
    rota[nCidades] = rota[0];
    
    
    for(int i = 0; i < nParticulas; i++){
        random_shuffle(rota.begin() + 1, rota.end() - 1, random_number);
        
        rota[nCidades] = rota[0];
        
        this->particulas.push_back(Particle(rota));
    }
}

Particle PSO::get_best()
{
    return this->best_particle;
}

void PSO::main_loop()
{   
    double f_value; //Resultado da função fitness
    double g_best_value; // Melhor resultado fitness da iteração
    Particle *g_best; // Melhor partícula da iteração

    for(int i = 0; i < nRep; i++){
        g_best_value = INFINITO;
        int t = 0;
        for(Particle& p: particulas){
            
            f_value = calcula_caminho(p.solucao_atual);
            
            // Atualiza o p_best da partícula i
            if(f_value < p.best_dist){
                p.best_dist = f_value;    
                p.p_best = p.solucao_atual;
            }
            // Atualiza o novo possível melhor global
            if(f_value < g_best_value){
                g_best_value = f_value;
                g_best = &p;
            }
        }
        // Atualiza o melhor resultado de todas as iterações
        if(g_best_value < this->best_particle.best_dist){
            this->best_particle = *g_best;
        }
        
        for(Particle& p: particulas){
            
            Particle p_best(p.p_best);
            
            double r1 = static_cast<double>(rand()) / RAND_MAX;
            double r2 = static_cast<double>(rand()) / RAND_MAX;
            
            double w = w_max - (w_max - w_min)/nRep * i;

            Velocity v = p.velocity * w + (p_best - p) * c1 * r1  + (*g_best - p) * c2 * r2;
            p.velocity = v;
            
            p.aplicar_velocidade(v);
        }
        
    }


}

double PSO::calcula_caminho(std::vector<int> caminho) //Fitness function
{
    double distancia = 0;
    int capcAtual = this->capacidadeV;

    for(int i = 0; i < nCidades; i++){

        if(this->cidades[caminho[i+1]].demanda > this->capacidadeV){
            throw invalid_argument("Cliente com demanda maior que o esperado!\n");
            
        }
        
        
        if(this->cidades[caminho[i+1]].demanda <= capcAtual){
            capcAtual -= this->cidades[caminho[i + 1]].demanda;

            distancia += distancias[caminho[i]][caminho[i+1]];
        }
        else{
            distancia += distancias[caminho[i]][0];
            capcAtual = this->capacidadeV;
            distancia += distancias[0][caminho[i+1]];
            capcAtual -= this->cidades[caminho[i+1]].demanda;
        }
        
    }
    return distancia;
}

double PSO::calcula_distancia(Cidade &a, Cidade &b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}