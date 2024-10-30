#include "Grafico.hpp"
#include <vector>
#include <utility>
#include <iostream>
#include <string>

void Grafico::apresentar(string instance_name, vector<Cidade> &c, vector<int> solucao)
{
    using namespace matplot;

    std::vector<std::pair<size_t, size_t>> arestas;
    std::vector<double> posicoes_x;
    std::vector<double> posicoes_y;
    std::vector<double> cores(c.size());

    double cor = 0;

    for(int i = 0; i < c.size(); i++){
        posicoes_x.push_back(c[i].x);
        posicoes_y.push_back(c[i].y);
    }
    cores.push_back(cor);

    for(int i = 0; i < solucao.size() - 1; i++){
        std::pair<size_t, size_t> aresta(solucao[i], solucao[i+1]);

        if(aresta.first != 0)
            cores[aresta.first] = cor;

        if(aresta.second == 0){
            cor += 0.1;
        }

        cout << "{" << aresta.first<< ", "<< aresta.second<< "}\n";
        arestas.push_back(aresta);
    }
    for(int i = 0; i < cores.size(); i++){
        std::cout << cores[i] << ", ";
    }
    std::cout<<"\n";
    std::cout<<cores.size()<<"\n";
    auto g = graph(arestas);
    
    title(instance_name);
    g->marker_colors(cores);
    g->x_data(posicoes_x);
    g->y_data(posicoes_y);
    view(2);
    show();
}
