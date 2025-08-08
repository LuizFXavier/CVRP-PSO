#include <utility>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include "Particle.hpp"

int Particle::capacidadeV = 0;
vector<Cidade> c;
vector<Cidade>& Particle::cidades = c;

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

        int a = 0;
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
Particle::get_full_solution() const {

    vector<int> sol;
    int capcAtual = capacidadeV;
    sol.push_back(0);

    for(int i = 0; i < cidades.size(); i++){

        if(cidades[this->solucao_atual[i+1]].demanda <= capcAtual){
            capcAtual -= cidades[this->solucao_atual[i + 1]].demanda;
            sol.push_back(this->solucao_atual[i+1]);
        }
        else{
            sol.push_back(0);
            capcAtual = capacidadeV;
            sol.push_back(this->solucao_atual[i+1]);
            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
        }

    }
    return sol;
}

vector<int>
Particle::get_full_solution(std::vector<std::pair<unsigned, unsigned>> &delm) const {
    vector<int> sol;
    int capcAtual = capacidadeV;
    sol.emplace_back(0);

    delm.emplace_back(0, 0);

    for(int i = 0; i < cidades.size(); i++){

        if(cidades[this->solucao_atual[i+1]].demanda <= capcAtual){
            capcAtual -= cidades[this->solucao_atual[i + 1]].demanda;
            sol.emplace_back(this->solucao_atual[i+1]);
        }
        else{
            delm.back().second = sol.size();
            if (i < cidades.size() - 1) {
                delm.emplace_back(sol.size(), 0);
            }
            sol.emplace_back(0);

            capcAtual = capacidadeV;
            sol.emplace_back(this->solucao_atual[i+1]);
            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
        }

    }
    delm.back().second = sol.size() - 1;
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

vector<Route>
Particle::get_routes()
{
    vector<Route> rotas{};
    int nRotas{};

    auto capcAtual = capacidadeV;

    // Rotas sempre começam com 0
    rotas.push_back(Route());

    for(int i = 0; i < solucao_atual.size() - 1; i++){

        // if(cidades[this->solucao_atual[i+1]].demanda > capcAtual){
        //
        //     // Considera volta ao depósito
        //     rotas[nRotas].custo +=Cidade::distancia(cidades, solucao_atual[i], 0);
        //
        //     rotas[nRotas].path.push_back(0); // Finaliza a rota
        //     rotas.push_back(Route());        // Inicia uma rota nova
        //     ++nRotas;                          // Incrementa o contador de rotas
        //     capcAtual = capacidadeV;           // Reseta a capacidade disponível
        // }
        //
        // capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
        //
        // rotas[nRotas].path.push_back(this->solucao_atual[i+1]);
        // rotas[nRotas].custo += Cidade::distancia(cidades, solucao_atual[i], solucao_atual[i+1]);
        // rotas[nRotas].demanda += cidades[solucao_atual[i+1]].demanda;


        if (cidades[this->solucao_atual[i+1]].demanda < capcAtual)
        {
            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
            rotas[nRotas].path.push_back(this->solucao_atual[i+1]);
            rotas[nRotas].custo += Cidade::distancia(cidades, solucao_atual[i], solucao_atual[i+1]);
            rotas[nRotas].demanda += cidades[solucao_atual[i+1]].demanda;
        }
        else
        {
            // Encerramento da rota
            capcAtual = capacidadeV;
            rotas[nRotas].path.push_back(0);
            rotas[nRotas].custo += Cidade::distancia(cidades, solucao_atual[i], 0);

            // Incialização da próxima rota
            rotas.push_back(Route());
            ++nRotas;

            capcAtual -= cidades[this->solucao_atual[i+1]].demanda;
            rotas[nRotas].path.push_back(this->solucao_atual[i+1]);
            rotas[nRotas].custo += Cidade::distancia(cidades, 0, solucao_atual[i+1]);
            rotas[nRotas].demanda += cidades[solucao_atual[i+1]].demanda;
        }
    }

    return rotas;
}

void 
Particle::update_tour(vector<Route> &r)
{
    int i = 1;
    double custo = 0;
    for(auto rota : r){
        std::copy(rota.path.begin()+1, rota.path.end()-1, solucao_atual.begin() + i);
        i += rota.path.size() - 2;
        custo += rota.custo;
    }
    if (custo < 0)
    {
        cout << "Soos\n";
    }
    atualiza_dist(custo);
}