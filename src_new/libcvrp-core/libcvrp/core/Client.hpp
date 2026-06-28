#pragma once

#include <vector>
#include  <cmath>

namespace cvrp
{

struct Client
{
    float x{};
    float y{};
    unsigned int demand{};
};

inline double 
distance(const Client& a, const Client& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
    
} // namespace cvrp



