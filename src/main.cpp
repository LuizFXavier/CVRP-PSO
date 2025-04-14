#include <iostream>
#include "PSO.hpp"
#include "CommandLine.hpp"
#include "Solucao.hpp"
#include "util.hpp"
using namespace std;

int main(int argc, char * argv[]){

    PSO pso;
    
    std::vector<std::vector<Solucao>> solucoes_particulas;

    CommandLine commandline(argc, argv, pso, solucoes_particulas);

    // cout << pso.instance_name << endl << pso.nCidades << endl;

    pso.executar(solucoes_particulas);

    cout << pso.get_best().best_dist << endl;

    if(pso.seguir_melhor){
        util::salva_solucoes(solucoes_particulas[0], pso.instance_name, pso.getNPart(), pso.getNRep(), -1);
    }

    for (int i = 1; i < solucoes_particulas.size(); i++){

        util::salva_solucoes(solucoes_particulas[i], pso.instance_name, pso.getNPart(), pso.getNRep(), i-1);
    }
    
    return EXIT_SUCCESS;
}