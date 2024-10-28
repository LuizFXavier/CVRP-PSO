#include <iostream>
// #include "Grafico.hpp"
#include "PSO.hpp"
using namespace std;

int main(int argc, char * argv[]){

    if(argc < 1){
        return EXIT_FAILURE;
    }
    
    PSO pso(argv[1]);
    
    pso.set_properties(argv[2]);

    cout << pso.instance_name << ":"<<endl;
    pso.executar();
    Particle best = pso.get_best();

    pso.apresentar(best);

    cout << pso.get_best().best_dist << endl;
    //Grafico::apresentar(pso.cidades, pso.get_solution(pso.get_best()));
    return EXIT_SUCCESS;
}