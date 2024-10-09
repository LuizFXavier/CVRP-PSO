#include <iostream>
#include "PSO.hpp"
using namespace std;

int main(int argc, char * argv[]){

    if(argc < 1){
        return EXIT_FAILURE;
    }
    
    PSO pso(argv[1]);

    pso.executar();
    for(int i = 0; i < pso.nParticulas; i++){
        pso.apresentar(pso.particulas[i]);
    }
    cout<<"\n-------------------------------------\n";

    return EXIT_SUCCESS;
}