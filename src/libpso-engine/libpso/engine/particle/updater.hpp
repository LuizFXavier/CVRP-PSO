#pragma once

#include <libcvrp/core/Instance.hpp>
#include <libpso/core/Particle.hpp>
#include <libpso/core/Velocity.hpp>

namespace pso
{
  // Operador de diferença entre soluções de partículas
  Velocity operator-(const Particle& p1, const Particle& p2);

  // Operador de soma para Velocidades
  Velocity operator+(const Velocity& v1, const Velocity& v2);
  
  // Operador de multiplicação por escalar
  Velocity operator*(const Velocity& v, float c);
  
  void apply_velocity(Particle& p, Velocity& v);

  float fitness(Particle& p, cvrp::Instance& instance);

} // namespace pso
