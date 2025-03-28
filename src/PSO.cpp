#include <fstream>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <regex>
#include "Cidade.hpp"
#include "PSO.hpp"
#include "Scanner.hpp"

PSO::PSO(string cities_file)
{
    srand(time(0)); //Seed para geração de números aleatórios posteriormente.
    ifstream c_file(cities_file);

    string line;
    regex reg(": ([\\w-]+)");
    smatch match;

    //Leitura do nome da instância
    getline(c_file, line);
    regex_search(line, match, reg);
    instance_name = match.str(1);

    //Leitura de partes desnecessárias
    getline(c_file, line); 
    getline(c_file, line);

    //Leitura da dimensão do problema (nCidadedes)
    getline(c_file, line); 
    regex_search(line, match, reg);
    nCidades = stoi(match.str(1));

    getline(c_file, line); //Parte desnecessária

    //Leitura da capacidade dos veículos
    getline(c_file, line);
    regex_search(line, match, reg);
    capacidadeV = stoi(match.str(1));
    
    //Início da sessão de coordenadas
    getline(c_file, line); 

    { //Leitura das cidades
        double x, y;
        int demanda;
        int i;

        for(c_file >> i ; i < nCidades ; c_file >> i){
            
            c_file >> x;
            c_file >> y;
            this->cidades.push_back(Cidade(x, y));
        }
        c_file >> x;
        c_file >> y;
        this->cidades.push_back(Cidade(x, y));

        //Início da sessão de demandas
        c_file >> line;
        
        for(c_file >> i ; i < nCidades ; c_file >> i){
            
            c_file >> demanda;
            
            this->cidades[i-1].demanda = demanda; 
        }
        c_file >> this->cidades[i-1].demanda;

        c_file >> line;

        c_file >> this->deposito;
    }

    c_file.close();
    this->best_particle.best_dist = INFINITO;
}

void PSO::set_properties(string config_file)
{
    unordered_map<string, string> configs = Scanner::read_config(config_file);

    if(configs.size() < 6){
        cout << configs.size() << endl;
        throw runtime_error("Número de parâmetros para o PSO não bate!");
    }

    this->nParticulas = stoi(configs["nParticulas"]);
    this->c1 = stod(configs["c1"]);
    this->c2 = stod(configs["c2"]);
    this->w_min = stod(configs["w_min"]);
    this->w_max = stod(configs["w_max"]);
    this->nRep = stoi(configs["nRep"]);
}

void PSO::set_properties(string nParticulas, string nRep)
{
    this->nParticulas = stoi(nParticulas);
    this->nRep = stoi(nRep);

    this->c1 = 0.1;
    this->c2 = 1;
    this->w_min = 1;
    this->w_max = 1;
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

            distancia += calcula_distancia(p.solucao_atual[i], p.solucao_atual[i+1]);

            sol.push_back(p.solucao_atual[i+1]);
        }
        else{
            distancia += calcula_distancia(p.solucao_atual[i], 0);
            sol.push_back(0);
            capcAtual = this->capacidadeV;

            distancia += calcula_distancia(0, p.solucao_atual[i+1]);
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

            distancia += calcula_distancia(p.solucao_atual[i], p.solucao_atual[i+1]);

            cout << p.solucao_atual[i+1]<< " ";
        }
        else{
            distancia += calcula_distancia(p.solucao_atual[i], 0);
            cout << "0 ";
            capcAtual = this->capacidadeV;

            distancia += calcula_distancia(0, p.solucao_atual[i+1]);
            cout << p.solucao_atual[i+1]<< " ";
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

            distancia += calcula_distancia(caminho[i], caminho[i+1]);;
        }
        else{
            distancia += calcula_distancia(caminho[i], 0);
            capcAtual = this->capacidadeV;
            distancia += calcula_distancia(0, caminho[i+1]);
            capcAtual -= this->cidades[caminho[i+1]].demanda;
        }
        
    }
    return distancia;
}

double PSO::calcula_distancia(int a, int b)
{
    
    return sqrt(pow(cidades[a].x - cidades[b].x, 2) + pow(cidades[a].y - cidades[b].y, 2));
}