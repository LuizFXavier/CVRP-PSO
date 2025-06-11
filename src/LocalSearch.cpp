#include <algorithm>
#include "LocalSearch.h"

#include "Cidade.hpp"

void
LocalSearch::melhoria_2_opt(Particle &p){
    int begin = 0;
    int desalinhamento = 0;

    auto solucao = p.get_full_solution(cidades, capacidadeV);

    for(int i = 1; i < solucao.size(); i++){
        if(solucao[i] == 0){
            melhorar_rota(solucao, p, begin, i, desalinhamento);
            begin = i;
            desalinhamento++;
        }
    }
    p.atualiza_dist(cidades, capacidadeV);

}

void
LocalSearch::melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des){
    bool improved = true;
    // cout << "sub-rota inicial: " << calcula_caminho(rota, begin, end) << endl;
    // for(int i = begin; i < end; i++){
    //     cout << rota[i] << "-";
    // }
    auto &c = cidades;
    while (improved) {
        improved = false;
        for (size_t i = begin + 1; i < end; ++i) {
            for (size_t j = i + 1; j < end; ++j) {
                // Calcula a distância antes da troca
                double old_dist = Cidade::distancia(c[rota[i-1]], c[rota[i]]) + Cidade::distancia(c[rota[j]], c[rota[(j+1)]]);
                // Distância após a troca (2-opt swap)
                double new_dist = Cidade::distancia(c[rota[i-1]], c[rota[j]]) + Cidade::distancia(c[rota[i]], c[rota[(j+1)]]);
                // cout << "dist: " << old_dist << "/" << new_dist << endl;
                // Se a nova distância for menor, aplica a troca
                if (new_dist < old_dist) {

                    std::reverse(rota.begin() + i, rota.begin() + j + 1);

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