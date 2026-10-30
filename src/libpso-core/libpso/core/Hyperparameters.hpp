#pragma once

namespace pso
{
  struct Hyperparameters
  {
    float cognitive_c = 1;  //Coeficiente cognitivo
    float social_c = 1;  //Coeficiente social
    float w_min = 0.1; //Coeficiente de inércia mínimo
    float w_max = 1; //Coeficiente de inércia máximo
    int iterations = 0;  //Número de iterações a serem performadas
    int swarm_size = 0; // Number of particles in the swarm
    int elite_size = -1; // Número de partículas do enxame que passarão pelos mecanismos de busca local
    int batch_size = 1;
    int sectorized = 0;
  };
  
} // namespace pso
