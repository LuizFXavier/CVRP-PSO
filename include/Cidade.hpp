#pragma once
#include <vector>
#include  <cmath>
class Cidade
{
public:
    double x, y;
    int demanda;
    Cidade(double x, double y):x(x), y(y) {};
    
    bool greater(Cidade* outra, bool cmpX){return cmpX ? this->x > outra->x : this->y > outra->y;};

    static auto distancia(Cidade &a, Cidade &b){return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));};
    static auto distancia(std::vector<Cidade> &v, int &a, int &b){return Cidade::distancia(v[a], v[b]);};
};