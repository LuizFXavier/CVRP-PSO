#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <string>
#include <utility>

// Grava, por execução:
//   <base>_init.csv  (1 linha)  qualidade e diversidade da população inicial
//   <base>_iter.csv  (1 linha por iteração do PSO)
//   <base>_hist.csv  (1 linha por chamada do swap*, só em algumas iterações)
class RunLogger
{
  using clk = std::chrono::steady_clock;

public:
  // base_path ex.: ".../X-n303-k21_c11.0_c21.0_sw250_el50_it250_run0"
  // bks: custo da melhor solução conhecida; se > 0, grava também os gaps iniciais
  explicit RunLogger(const std::string& base_path,
                     double bks = 0.0,
                     std::set<int> hist_iters = {0, 25, 50})
    : base_(base_path), bks_(bks), hist_iters_(std::move(hist_iters))
  {
    auto parent = std::filesystem::path(base_path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);

    iter_file_.open(base_path + "_iter.csv");
    hist_file_.open(base_path + "_hist.csv");

    iter_file_ << std::setprecision(10);
    iter_file_ << "iter,g_best,calls_used,n_last,hit_cap,iter_ms,"
                  "swap_star_ms,two_opt_ms,import_ms,cum_ms,"
                  "distinct_swarm,frac_eq_gbest\n";
    hist_file_ << "iter,call,n_improved\n";

    t_start_ = t_prev_ = clk::now();
  }

  // Chamar uma vez, logo após a inicialização das partículas.
  //   best_cost / mean_cost: fitness da população inicial, antes de qualquer busca local
  //   distinct: quantas soluções iniciais distintas
  void log_init(int swarm_size, double best_cost, double mean_cost, int distinct)
  {
    std::ofstream f(base_ + "_init.csv");
    f << std::setprecision(10);
    f << "swarm,init_best_cost,init_mean_cost,init_gap,init_mean_gap,distinct_init\n";
    f << swarm_size << ',' << best_cost << ',' << mean_cost << ',';
    if (bks_ > 0) f << (best_cost / bks_ - 1.0) * 100.0;
    f << ',';
    if (bks_ > 0) f << (mean_cost / bks_ - 1.0) * 100.0;
    f << ',' << distinct << '\n';
    f.flush();

    // o relógio da primeira iteração não deve incluir o cálculo acima
    t_start_ = t_prev_ = clk::now();
  }

  // Chamar no fim de cada iteração do PSO.
  //   distinct_swarm < 0  -> não medido nesta iteração (coluna vazia)
  //   frac_eq_gbest  < 0  -> idem
  template <class PhaseTimes>
  void log_iteration(int iter, double g_best, int distinct_swarm, double frac_eq_gbest,
                     PhaseTimes& t)
  {
    auto now = clk::now();
    double iter_ms = ms(t_prev_, now);
    double cum_ms  = ms(t_start_, now);
    t_prev_ = now;

    // PhaseTimes acumula; o que interessa é o delta desta iteração
    double d_swap = t.swap_star - prev_swap_;   prev_swap_   = t.swap_star;
    double d_two  = t.two_opt   - prev_two_;    prev_two_    = t.two_opt;
    double d_imp  = t.import    - prev_import_; prev_import_ = t.import;

    int n_last = t.improved_hist.empty() ? 0 : t.improved_hist.back();

    iter_file_ << iter << ',' << g_best << ',' << t.calls_used << ',' << n_last << ','
               << (t.hit_cap ? 1 : 0) << ',' << iter_ms << ',' << d_swap << ','
               << d_two << ',' << d_imp << ',' << cum_ms << ',';
    if (distinct_swarm >= 0) iter_file_ << distinct_swarm;
    iter_file_ << ',';
    if (frac_eq_gbest >= 0) iter_file_ << frac_eq_gbest;
    iter_file_ << '\n';
    iter_file_.flush();

    if (hist_iters_.count(iter)) {
      for (size_t c = 0; c < t.improved_hist.size(); ++c)
        hist_file_ << iter << ',' << c << ',' << t.improved_hist[c] << '\n';
      hist_file_.flush();
    }

    // zera o que é por chamada de optimize (caso a próxima iteração não chame)
    t.calls_used = 0;
    t.hit_cap = false;
    t.improved_hist.clear();
  }

private:
  static double ms(clk::time_point a, clk::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
  }

  std::string base_;
  double bks_;
  std::set<int> hist_iters_;
  std::ofstream iter_file_, hist_file_;
  clk::time_point t_start_, t_prev_;
  double prev_swap_ = 0, prev_two_ = 0, prev_import_ = 0;
};