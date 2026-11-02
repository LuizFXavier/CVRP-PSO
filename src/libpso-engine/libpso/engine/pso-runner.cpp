#include <vector>
#include <stdexcept>
#include <format>
#include <algorithm>
#include <chrono>
#include <unordered_set>

#include <libpso/engine/pso-runner.hpp>

#include <libcvrp/engine/local-search.hpp>
#include <libpso/engine/particle/initializer.hpp>
#include <libpso/engine/particle/updater.hpp>

namespace pso
{

namespace {
  uint64_t hash_solution(const std::vector<int>& s)   // FNV-1a
  {
    uint64_t h = 1469598103934665603ull;
    for (int v : s) { h ^= static_cast<uint64_t>(static_cast<uint32_t>(v)); h *= 1099511628211ull; }
    return h;
  }

  pso::SwarmStats swarm_stats(const std::vector<Particle>& particles, const Particle& g_best)
  {
    std::unordered_set<uint64_t> seen;
    const uint64_t gh = hash_solution(g_best.curr_solution);
    size_t eq = 0;
    for (const auto& p : particles) {
      uint64_t h = hash_solution(p.curr_solution);
      seen.insert(h);
      if (h == gh) ++eq;
    }
    return { static_cast<int>(seen.size()), static_cast<double>(eq) / particles.size() };
  }
}

Particle 
run_pso(cvrp::Instance &instance, 
        Hyperparameters hyperparameters, 
        OptimizerFunc optimizer, 
        pso::PartialCost& costs, 
        pso::PhaseTimes& times,
        const Observers* obs)
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

  using clk = std::chrono::steady_clock;
  auto ms = [](clk::time_point a, clk::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  };

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

      particles.push_back(sector_initialize(instance, gen, !(hyperparameters.fixed_sector)));

    }

    // Inicialização aleatória
    for (int i = 0; i < hyperparameters.swarm_size - sectorized_particles; ++i){

      particles.push_back(random_initialize(instance.dimension, gen));
    }
  }

  if (obs && obs->on_init) {
    double best = std::numeric_limits<double>::max(), sum = 0;
    std::unordered_set<uint64_t> seen;
    for (auto& p : particles) {
      double f = fitness(p, instance);
      best = std::min(best, f);
      sum += f;
      seen.insert(hash_solution(p.curr_solution));
    }
    obs->on_init({ (int)particles.size(), best, sum / particles.size(), (int)seen.size() });
  }

  float g_best_of = cvrp::INF_F;

  Particle g_best;

  auto min_particle_cmp = [](const Particle* a, const Particle* b) { return a->curr_of > b->curr_of; };
  auto max_particle_cmp = [](const Particle* a, const Particle* b) { return a->curr_of < b->curr_of; };

  struct IterMark
  {
    int first, quarter, half, three_fourths;
  } iter_marks;

  iter_marks.first = 0;
  iter_marks.quarter = hyperparameters.iterations * 0.25;
  iter_marks.half = hyperparameters.iterations * 0.5;
  iter_marks.three_fourths = hyperparameters.iterations * 0.75;
  
  // Loop principal do PSO
  for (int i = 0; i < hyperparameters.iterations; ++i){

    SwarmStats st;
    
    std::vector<Particle*> elite;

    auto t0 = clk::now();

    // Atualização do fitness de cada partícula
    for (auto& p : particles) {
      p.curr_of = fitness(p, instance);

      // Definição da elite das particulas
      if (hyperparameters.elite_size <= 0)
        continue;

      if (elite.size() < hyperparameters.elite_size){
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

    auto t1 = clk::now();

    // Execução dos mecanismos de busca local nas partículas pertencentes à elite

    if (!elite.empty()) {
      std::vector<std::vector<int>*> elite_content;
      elite_content.reserve(elite.size());

      for (auto* p : elite) {
        elite_content.push_back(&(p->curr_solution));
      }

      // Envia a elite inteira de uma só vez para o otimizador (GPU/Batch)
      optimizer(elite_content, instance, times.optimizer);

      // Atualiza o fitness de todos os membros da elite após a otimização
      for (auto* p : elite) {
        p->curr_of = fitness(*p, instance);
      }
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

    if (obs && obs->stats_every > 0 && i % obs->stats_every == 0)
      st = swarm_stats(particles, g_best);

    if (i == iter_marks.first){
      costs.initial = g_best.best_of;
    }
    else if (i == iter_marks.quarter){
      costs.quarter = g_best.best_of;
    }
    else if (i == iter_marks.half){
      costs.half = g_best.best_of;
    }
    else if (i == iter_marks.three_fourths){
      costs.three_fourths = g_best.best_of;
    }
    
    float w = hyperparameters.w_max - (static_cast<float>(i) / hyperparameters.iterations) * (hyperparameters.w_max - hyperparameters.w_min);
    
    auto c1 = hyperparameters.cognitive_c;
    auto c2 = hyperparameters.social_c;
    
    // Distribuição uniforme para gerar r1 e r2
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    auto t2 = clk::now();

    // Atualização da velocidade de cada partícula
    for (int j = 0; j < particles.size(); ++j){

      auto& p = particles[j];

      Particle p_best;

      p_best.curr_solution = p.best_solution;

      float r1 = dist(gen);
      float r2 = dist(gen);

      p.velocity = p.velocity * w + (p_best - p) * (c1 * r1) + (g_best - p) * (c2 * r2);
    }

    auto t3 = clk::now();

    // Atualização da posição de cada partícula
    for (int j = 0; j < particles.size(); ++j){

      auto& p = particles[j];

      apply_velocity(p, p.velocity);

    }

    auto t4 = clk::now();

    times.fitness_calculation += ms(t0, t1);
    times.velocity_update     += ms(t2, t3);
    times.position_update     += ms(t3, t4);

    if (obs && obs->on_iteration) obs->on_iteration({ i, g_best_of, st });
  }

  costs.final = g_best.best_of;


  return g_best;
}
  
} // namespace pso

