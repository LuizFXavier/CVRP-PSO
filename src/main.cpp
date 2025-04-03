#include <iostream>
#include "PSO.hpp"
#include "CommandLine.hpp"
using namespace std;

int main(int argc, char * argv[]){

    PSO pso;
    
    CommandLine commandline(argc, argv, pso);

    // cout << pso.instance_name << endl << pso.nCidades << endl;

    pso.executar();

    cout << pso.get_best().best_dist << endl;
    
    return EXIT_SUCCESS;
}