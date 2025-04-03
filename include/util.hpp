#pragma once
#include <cmath>
#include <chrono>
#include "Cidade.hpp"

namespace util
{
    inline double
    calcula_distancia(Cidade &a, Cidade &b){ 
        return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
    }

    unsigned time_seed = std::chrono::system_clock::now().time_since_epoch().count();
} // namespace util
