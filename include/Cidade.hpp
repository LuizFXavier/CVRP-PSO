#pragma once
#include <vector>
class Cidade
{
public:
    double x, y;
    int demanda;
    Cidade(double x, double y):x(x), y(y) {};
    
    bool greater(Cidade* outra, bool cmpX){return cmpX ? this->x > outra->x : this->y > outra->y;};
};