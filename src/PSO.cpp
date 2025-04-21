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
#include "util.hpp"

unsigned time_seed = std::chrono::system_clock::now().time_since_epoch().count();

void PSO::set_instance(string cities_file){
    ifstream c_file(cities_file);

    string line;
    
    smatch match;
    string chave;

    regex word_regex("[\\w-]+");
    regex number_regex("[0-9]*\\.?[0-9]+");
    int a = 0;
    do{
        getline(c_file, line);
        cout << line << endl;
        
        regex_search(line, match, word_regex);
        
        chave = match.str();

        if(chave == "NAME"){
            line = match.suffix();
            regex_search(line, match, word_regex);
            instance_name = match.str();
        }
        else if (chave == "DIMENSION"){
            regex_search(line, match, number_regex);
            nCidades = stoi(match.str());
        }
        else if (chave == "CAPACITY"){
            regex_search(line, match, number_regex);
            capacidadeV = stoi(match.str());
        }
    } while (chave != "NODE_COORD_SECTION");
    cout << "Vai começar a ler as coordenadas\n";
    { //Leitura das coordenadas dos clientes
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
        cout << "Terminou as coordenadas\n";
        //Início da seção de demandas
        c_file >> line;
        
        for(c_file >> i ; i < nCidades ; c_file >> i){
            
            c_file >> demanda;
            
            this->cidades[i-1].demanda = demanda; 
        }
        c_file >> this->cidades[i-1].demanda;

        c_file >> line;

        c_file >> this->deposito;
    }
    cout << "Terminou as demands\n";
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

void PSO::executar(string routes_file){
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

    // main_loop();

}

void PSO::melhoria_2_opt(Particle &p){
    int begin = 0;
    int desalinhamento = 0;
    cout << "rotas iniciais: " << calcula_caminho(p.solucao_atual) << endl;
    auto solucao = get_solution(p);

    for(int i = 1; i < solucao.size(); i++){
        if(solucao[i] == 0){
            melhorar_rota(solucao, p, begin, i, desalinhamento);
            begin = i;
            desalinhamento++;
        }
    }
    p.dist_atual = calcula_caminho(p.solucao_atual);

    if(p.best_dist > p.dist_atual){
        p.best_dist = p.dist_atual;
        p.p_best = p.solucao_atual;
    }
    
}

void PSO::melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des){
    bool improved = true;
    // cout << "sub-rota inicial: " << calcula_caminho(rota, begin, end) << endl;
    // for(int i = begin; i < end; i++){
    //     cout << rota[i] << "-";
    // }
    cout << endl;
    while (improved) {
        improved = false;
        for (size_t i = begin + 1; i < end; ++i) {
            for (size_t j = i + 1; j < end; ++j) {
                // Calcula a distância antes da troca
                double old_dist = calcula_distancia(rota[i-1], rota[i]) + calcula_distancia(rota[j], rota[(j+1)]);
                // Distância após a troca (2-opt swap)
                double new_dist = calcula_distancia(rota[i-1], rota[j]) + calcula_distancia(rota[i], rota[(j+1)]);
                // cout << "dist: " << old_dist << "/" << new_dist << endl;
                // Se a nova distância for menor, aplica a troca
                if (new_dist < old_dist) {
                    cout << "Trocou! " << i << " e " << j << endl;
                    // for(int i = begin; i < end; i++){
                    //     cout << rota[i] << "-";
                    // }
                    // cout << endl;
                    std::reverse(rota.begin() + i, rota.begin() + j + 1);
                    // for(int i = begin; i < end; i++){
                    //     cout << rota[i] << "-";
                    // }
                    // cout << endl;
                    improved = true;
                }
            }
        }
    }
    for(int i = begin + 1; i < end; i++){
        p.solucao_atual[i - des] = rota[i];
    }
    // cout << "sub-rota otimizada: " << calcula_caminho(rota, begin, end) << endl;
    // cout << "sub-rota inicial: " << calcula_caminho(rota, begin, end) << endl;
    // for(int i = begin; i < end; i++){
    //     cout << rota[i] << "-";
    // }
    // cout << endl << endl;
}

vector<int> PSO::get_solution(Particle &p)
{
    double distancia = 0;
    vector<int> sol;
    int capcAtual = this->capacidadeV;
    sol.push_back(0);

    for(int i = 0; i < nCidades; i++){
        
        if(this->cidades[p.solucao_atual[i+1]].demanda <= capcAtual){
            
            capcAtual -= this->cidades[p.solucao_atual[i + 1]].demanda;

            distancia += calcula_distancia(cidades[p.solucao_atual[i]], cidades[p.solucao_atual[i+1]]);

            sol.push_back(p.solucao_atual[i+1]);
        }
        else{
            distancia += calcula_distancia(cidades[p.solucao_atual[i]], cidades[0]);
            sol.push_back(0);
            capcAtual = this->capacidadeV;

            distancia += calcula_distancia(cidades[0], cidades[p.solucao_atual[i+1]]);
            sol.push_back(p.solucao_atual[i+1]);
            capcAtual -= this->cidades[p.solucao_atual[i+1]].demanda;
        }
        
    }
    return sol;
}

void PSO::apresentar(Particle &p){
    double distancia = 0;

    auto sol = get_solution(p);

    for(int i = 0; i < sol.size() - 1; i++){
        
        distancia += calcula_distancia(cidades[sol[i]], cidades[sol[i+1]]);
        cout << sol[i] << " ";
    }
    cout << sol[sol.size() - 1];

    cout << "\n" << distancia << "\n";
}

void PSO::gerar_particulas(){
    vector<int> rota(nCidades + 1);
    for(int i = 0; i < nCidades; i++){
        rota[i] = i;
    }
    rota[nCidades] = rota[0];
    
    
    for(int i = 0; i < nParticulas; i++){
        shuffle(rota.begin() + 1, rota.end() - 1, default_random_engine(time_seed));
        
        rota[nCidades] = rota[0];
        
        this->particulas.push_back(Particle(rota));
    }
}

void PSO::main_loop(std::vector<std::vector<Solucao>> &solucoes){

    double f_value; //Resultado da função fitness
    Particle *g_best; // Melhor partícula da iteração
    auto particle_cmp = [](const Particle* a, const Particle* b) { return a->dist_atual > b->dist_atual; };

    for(int i = 0; i < nRep; i++){
        int nSalvas = 1;
        vector<Particle*> elite;

        for(Particle& p: particulas){
            
            f_value = calcula_caminho(p.solucao_atual);
            p.dist_atual = f_value;
            // Atualiza o p_best da partícula i
            if(f_value < p.best_dist){
                p.best_dist = f_value;    
                p.p_best = p.solucao_atual;
            }

            if(seguir_qualquer > 0 && i % seguir_qualquer == 0 && nSalvas < solucoes.size()){
                util::guarda_solucao(solucoes[nSalvas++], i, f_value, get_solution(p));
            }

            if(elite.size() == 0 || (tam_elite != 0 && elite.size() < tam_elite)){
                elite.push_back(&p);
                make_heap(elite.begin(), elite.end());
            }
            else{
                if(p.dist_atual < elite[0]->dist_atual){
                    elite[0] = &p;
                    make_heap(elite.begin(), elite.end());
                }
            }
        }
        

        if(seguir_melhor > 0 && i % seguir_melhor == 0){
            util::guarda_solucao(solucoes[0], i, elite[0]->dist_atual, get_solution(*elite[0]));
        }
        // Aplica a heurística de melhoria

        for(int e = 0; e < elite.size(); e++){
            melhoria_2_opt(*elite[e]);
        }
        make_heap(elite.begin(), elite.end(), particle_cmp);

        if(seguir_melhor > 0 && i % seguir_melhor == 0){
            util::guarda_solucao(solucoes[0], i, elite[0]->dist_atual, get_solution(*elite[0]));
        }

        // Atualiza o melhor resultado de todas as iterações

        if(elite[0]->dist_atual < this->best_particle.best_dist){
            this->best_particle = *elite[0];
        }
        
        g_best = elite[0];
        
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

double PSO::calcula_caminho(std::vector<int> caminho){ //Fitness function
    double distancia = 0;
    int capcAtual = this->capacidadeV;

    for(int i = 0; i < nCidades; i++){

        if(this->cidades[caminho[i+1]].demanda > this->capacidadeV){
            throw invalid_argument("Cliente com demanda maior que o esperado!\n");
            
        }
        
        if(this->cidades[caminho[i+1]].demanda <= capcAtual){
            capcAtual -= this->cidades[caminho[i + 1]].demanda;

            distancia += calcula_distancia(this->cidades[caminho[i]], this->cidades[caminho[i+1]]);
        }
        else{
            distancia += calcula_distancia(this->cidades[caminho[i]], this->cidades[0]);
            capcAtual = this->capacidadeV;
            distancia += calcula_distancia(this->cidades[0], this->cidades[caminho[i+1]]);
            capcAtual -= this->cidades[caminho[i+1]].demanda;
        }
        
    }
    return distancia;
}

double PSO::calcula_caminho(vector<int> caminho, int begin, int end){
    double distancia = 0;
    for(int i = begin; i < end; i++){
        distancia += calcula_distancia(cidades[i], cidades[i+1]);
    }
    return distancia;
}