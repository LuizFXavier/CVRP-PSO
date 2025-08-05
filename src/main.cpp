#include <iostream>
#include "PSO.hpp"
#include "CommandLine.hpp"
#include "Solucao.hpp"
#include "util.hpp"
using namespace std;

void teste_troca(){
    Particle p(vector<int>{0,1,2,3,4,5,6,7,0});

    vector<Route> rotas{};

    rotas.push_back(Route());
    rotas.push_back(Route());

    rotas[0].path = vector<int>{0,9,9,9,9,0};
    rotas[1].path = vector<int>{0,8,8,8,0};

    p.update_tour(rotas);

    for(int i : p.solucao_atual)
        cout << i << ", ";
    cout << endl;
}

int main(int argc, char * argv[]){

    // teste_troca();

    PSO pso;
    
    std::vector<std::vector<Solucao>> solucoes_particulas;
    solucoes_particulas.resize(1);
    CommandLine commandline(argc, argv, pso, solucoes_particulas);

    // cout << pso.instance_name << endl << pso.nCidades << endl;

    pso.executar(solucoes_particulas);

    // cout << pso.get_best().best_dist << endl;

    if(pso.seguir_melhor){
        util::salva_solucoes(solucoes_particulas[0], pso.instance_name, pso.getNPart(), pso.getNRep(), -1, pso.setorizar, pso.tam_elite);
    }

    for (int i = 1; i < solucoes_particulas.size(); i++){

        util::salva_solucoes(solucoes_particulas[i], pso.instance_name, pso.getNPart(), pso.getNRep(), i-1, pso.setorizar, pso.tam_elite);
    }
    
    return EXIT_SUCCESS;
}