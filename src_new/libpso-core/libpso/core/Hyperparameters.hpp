#pragma once

namespace pso
{
  struct Hyperparameters
  {
    double cognitive_c = 1;  //Coeficiente cognitivo
    double social_c = 1;  //Coeficiente social
    double w_min = 0.1; //Coeficiente de inércia mínimo
    double w_max = 1; //Coeficiente de inércia máximo
    int iterations = 0;  //Número de iterações a serem performadas
    int swarm_size = 0; // Number of particles in the swarm
    int elite = 0; // Número de partículas do enxame que passarão pelos mecanismos de busca local
  };
  
} // namespace pso
