#include <string>
#include <fstream>
#include "util.hpp"
#include "Solucao.hpp"

void
util::salva_solucoes(std::vector<Solucao> &solutions, std::string instance_name, int nPart, int nRep, int qual){

    std::string part = qual < 0 ? "_best" : std::string("_") + std::to_string(qual);

    std::string nome_arquivo = "./resultado/";
    nome_arquivo += instance_name;
    nome_arquivo += part;
    nome_arquivo += std::string("_nPart-") + to_string(nPart);
    nome_arquivo += std::string("_nRep-") + to_string(nRep);
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