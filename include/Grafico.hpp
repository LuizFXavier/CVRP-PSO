#pragma once
#include <matplot/matplot.h>
#include <cmath>
#include <string>
#include "Cidade.hpp"
#include "Particle.hpp"

class Grafico
{
private:
    /* data */
public:
    static void apresentar(string instance_name, vector<Cidade> &c, vector<int> solucao);

};
