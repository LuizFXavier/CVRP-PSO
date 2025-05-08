#include <string>
#include <fstream>
#include <random>
#include "util.hpp"
#include "Solucao.hpp"

int 
util::numero_aleatorio(int n) {
    // Dispositivo aleatório
    std::random_device rd;
    
    // Gerador de números pseudoaleatórios
    std::mt19937 gen(rd());
    
    // Distribuição uniforme entre 1 e n (inclusive)
    std::uniform_int_distribution<> distrib(1, n);
    
    return distrib(gen);
}

void
util::salva_solucoes
(std::vector<Solucao> &solutions, std::string instance_name, int nPart, int nRep, int qual, bool set, int elite){

    std::string part = qual < 0 ? "_best" : std::string("_") + std::to_string(qual);

    std::string nome_arquivo = "./resultado/";
    nome_arquivo += instance_name;
    nome_arquivo += part;
    nome_arquivo += std::string("_nPart-") + to_string(nPart);
    nome_arquivo += std::string("_nRep-") + to_string(nRep);
    nome_arquivo += std::string("_elt-") + to_string(elite);
    if(set){
        nome_arquivo += std::string("_setor");
    }
    nome_arquivo += std::string(".csv");

    std::ofstream arquivo(nome_arquivo);

    arquivo << "Iteração;Custo;Rota\n";
    
    for (const auto& sol : solutions) {
        arquivo << sol.iteracao << ";" << sol.custo << ";";
        
        // Escreve a rota
        for (size_t i = 0; i < sol.rota.size(); ++i) {
            if (i != 0) arquivo << "-";
            arquivo << sol.rota[i];
        }
        arquivo << "\n";
    }
    arquivo.close();
}