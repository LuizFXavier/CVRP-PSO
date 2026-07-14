#include <vector>
#include <stdexcept>
#include <format>
#include <algorithm>
#include <omp.h>

#include <libpso/engine/pso-runner.hpp>

#include <libcvrp/engine/local-search.hpp>
#include <libpso/engine/particle/initializer.hpp>
#include <libpso/engine/particle/updater.hpp>

namespace pso
{

Particle 
run_pso(cvrp::Instance &instance, Hyperparameters hyperparameters)
{
  // Verificação de erros
  {
    if (hyperparameters.swarm_size <= 0){
      throw std::runtime_error("Swarm size must be greater than zero.");
    }

    if (hyperparameters.iterations < 0) {
      throw std::invalid_argument("Number of iterations must not be negative.");
    }
    
    if (instance.dimension == 0 || instance.clients.empty()) {
      throw std::invalid_argument("CVRP instance is empty or invalid.");
    }
  }

  std::vector<pso::Particle> particles;

  // Criação de gerador de números aleatórios
  std::random_device rd;
  std::mt19937 gen(rd());

  // Inicialização das partícula
  {
    int sectorized_particles = std::min(hyperparameters.swarm_size, hyperparameters.sectorized);

    sectorized_particles = std::max(sectorized_particles, 0);

    // Inicialização setorizada
    for (int i = 0; i < sectorized_particles; ++i){

      particles.push_back(sector_initialize(instance, gen));

    }

    // Inicialização aleatória
    for (int i = 0; i < hyperparameters.swarm_size - sectorized_particles; ++i){

      particles.push_back(random_initialize(instance.dimension, gen));
    }
  }

  float g_best_of = cvrp::INF_F;

  Particle g_best;

  std::vector<Particle*> elite;

  auto min_particle_cmp = [](const Particle* a, const Particle* b) { return a->curr_of > b->curr_of; };
  auto max_particle_cmp = [](const Particle* a, const Particle* b) { return a->curr_of < b->curr_of; };
  
  // Loop principal do PSO
  for (int i = 0; i < hyperparameters.iterations; ++i){
    
    // Atualização do fitness de cada partícula
    for (auto& p : particles) {
      p.curr_of = fitness(p, instance);

      // Definição da elite das particulas

      if (elite.size() <= hyperparameters.elite_size){
        elite.push_back(&p);
        std::push_heap(elite.begin(), elite.end(), max_particle_cmp);
      }
      else if (p.curr_of < elite.front()->curr_of){

        std::pop_heap(elite.begin(), elite.end(), max_particle_cmp);
        elite.pop_back();

        elite.push_back(&p);
        std::push_heap(elite.begin(), elite.end(), max_particle_cmp);
      }
    }

    // Execução dos mecanismos de busca local nas partículas pertencentes à elite
    #pragma omp parallel for
    for (auto& e : elite){
      cvrp::local_search::optimize(e->curr_solution, instance);

      e->curr_of = fitness(*e, instance);
    }

    // Iteração para atualização de p_best e g_best
    for (int j = 0; j < hyperparameters.swarm_size; ++j){
      
      Particle& p = particles[j];

      // Possível atualização do p_best
      if (p.curr_of < p.best_of){

        p.best_of = p.curr_of;
        p.best_solution = p.curr_solution;

        // Possível atualização do g_best
        if (p.curr_of < g_best_of){
          g_best_of = p.curr_of;

          g_best = p;
        }
      }
    }
    
    auto w = (hyperparameters.w_max - hyperparameters.w_min) / (hyperparameters.iterations * i);

    auto c1 = hyperparameters.cognitive_c;
    auto c2 = hyperparameters.social_c;

    // Distribuição uniforme para gerar r1 e r2
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Atualização da velocidade de cada partícula
    for (int j = 0; j < particles.size(); ++j){

      auto& p = particles[j];

      Particle p_best;

      p_best.curr_solution = p.best_solution;

      float r1 = dist(gen);
      float r2 = dist(gen);

      p.velocity = p.velocity * w + (p_best - p) * (c1 * r1) + (g_best - p) * (c2 * r2);
    }

    // Atualização da posição de cada partícula
    for (int j = 0; j < particles.size(); ++j){

      auto& p = particles[j];

      apply_velocity(p, p.velocity);

    }
  }

  return g_best;
}
  
} // namespace pso

