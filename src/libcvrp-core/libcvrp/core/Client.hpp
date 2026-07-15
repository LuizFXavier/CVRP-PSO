#pragma once

#include <vector>
#include <cmath>

namespace cvrp
{

struct Client
{
  float x{};
  float y{};
  unsigned int demand{};
};

inline float 
distance(const Client& a, const Client& b)
{
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}
    
} // namespace cvrp



