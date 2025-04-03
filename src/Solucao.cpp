#include <string>
#include <fstream>
#include "Solucao.hpp"

std::vector<solucao::Solucao> solucao::melhor_solucao;

void
solucao::salva_solucoes(std::string instance_name, int nPart, int nRep){

    std::string nome_arquivo = instance_name;
    nome_arquivo += std::string("_nPart-") + to_string(nPart);
    nome_arquivo += std::string("_nRep-") + to_string(nRep);
    nome_arquivo += std::string(".csv");

    std::ofstream arquivo(nome_arquivo);

    arquivo << "Iteração;Custo;Rota\n";
    
    for (const auto& sol : solucao::melhor_solucao) {
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