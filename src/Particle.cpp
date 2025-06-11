#include <utility>
#include <iostream>
#include <unordered_map>
#include "Particle.hpp"

Velocity Particle::operator-(Particle &p1)
{
    Velocity v;

    unordered_map<int, int> positions;

    vector<int> caminho_aux = p1.solucao_atual;

    for(int i = 0; i < p1.solucao_atual.size() -1; i++){
        
        positions[this->solucao_atual[i]] = i;
    }
    
    for(int i = 0; i < p1.solucao_atual.size(); i++){
        int t = this->solucao_atual[i];

        while(t != caminho_aux[i]){
            
            int k = positions[caminho_aux[i]];
            v.value.push_back(pair(i, k));
            int temp = caminho_aux[i];
            caminho_aux[i] = caminho_aux[k];
            caminho_aux[k] = temp;

            if(i == 0)
                caminho_aux[caminho_aux.size() -1] = caminho_aux[0];
            
        }
    }

    return v;
}

void Particle::aplicar_velocidade(Velocity &v)
{
    int aux;
    for(int i = 0; i < v.value.size(); i++){
        
        aux = this->solucao_atual[v.value[i].first];
        this->solucao_atual[v.value[i].first] = this->solucao_atual[v.value[i].second];
        this->solucao_atual[v.value[i].second] = aux;
        
        if(v.value[i].first == 0){
            this->solucao_atual[solucao_atual.size() -1] = this->solucao_atual[v.value[i].first];
        }
        else if(v.value[i].second == 0){
            this->solucao_atual[solucao_atual.size() -1] = this->solucao_atual[v.value[i].second];
        }
    }
}

vector<int>
Particle::get_full_solution(vector<Cidade> &cidades, int capacidade) {

    vector<int> sol;
    int capcAtual = capacidade;
    sol.push_back(0);

    for(int i = 0; i < cidades.size(); i++){

        if(cidades[this->solucao_atual[i+1]].demanda <= capcAtual){
            capcAtual -= cidades[this->solucao_atual[i + 1]].demanda;
            sol.push_back(this->solucao_atual[i+1]);
        }
        else{
            sol.push_back(0);
            capcAtual = capacidade;
            sol.push_back(this->solucao_atual[i+1]);
            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
        }

    }
    return sol;
}

double
Particle::fitness(vector<Cidade> &cidades, int capacidade) {

    double distancia = 0;
    int capcAtual = capacidade;

    for(int i = 0; i < cidades.size(); i++){

        if(cidades[this->solucao_atual[i+1]].demanda <= capcAtual){

            capcAtual -= cidades[this->solucao_atual[i + 1]].demanda;
            distancia += Cidade::distancia(cidades[this->solucao_atual[i]], cidades[this->solucao_atual[i+1]]);
        }
        else{
            distancia += Cidade::distancia(cidades[this->solucao_atual[i]], cidades[0]);
            capcAtual = capacidade;
            distancia += Cidade::distancia(cidades[0], cidades[this->solucao_atual[i+1]]);
            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
        }

    }
    return distancia;
}
