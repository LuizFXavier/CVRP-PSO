#include <iostream>
#include "Grafico.hpp"
#include "PSO.hpp"
using namespace std;

int main(int argc, char * argv[]){

    if(argc < 1){
        return EXIT_FAILURE;
    }
    
    PSO pso(argv[1]);

    pso.executar();
    
    cout << pso.get_best().best_dist << endl;
    Grafico::apresentar(pso.cidades, pso.get_solution(pso.get_best()));
    return EXIT_SUCCESS;
}