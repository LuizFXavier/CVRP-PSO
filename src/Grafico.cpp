#include "Grafico.hpp"
#include <vector>
#include <utility>
#include <iostream>

void Grafico::apresentar(vector<Cidade> &c, vector<int> solucao)
{
    using namespace matplot;

    std::vector<std::pair<size_t, size_t>> arestas;
    std::vector<double> posicoes_x;
    std::vector<double> posicoes_y;

    for(int i = 0; i < c.size(); i++){
        posicoes_x.push_back(c[i].x);
        posicoes_y.push_back(c[i].y);
    }

    for(int i = 0; i < solucao.size() - 1; i++){
        std::pair<size_t, size_t> aresta(solucao[i], solucao[i+1]);

        if (aresta.first > aresta.second){
            size_t aux = aresta.first;
            aresta.first = aresta.second;
            aresta.second = aux;
        }

        cout << "{" << aresta.first << ", "<< aresta.second << "}\n";
        arestas.push_back(aresta);
    }

    auto g = graph(arestas);

    g->x_data(posicoes_x);
    g->y_data(posicoes_y);
    view(2);
    show();
}
