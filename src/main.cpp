#include <iostream>
#include "PSO.hpp"
using namespace std;

int main(int argc, char * argv[]){

    if(argc < 4){
        return EXIT_FAILURE;
    }
    
    PSO pso(argv[1]);
    
    pso.set_properties(argv[2], argv[3]);

//    cout << pso.instance_name <<endl;
    pso.executar();
    Particle best = pso.get_best();

//    pso.apresentar(best);

    cout << pso.get_best().best_dist << endl;
    
    return EXIT_SUCCESS;
}