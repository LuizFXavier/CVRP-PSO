#include <algorithm>
#include <unordered_map>
#include <iostream>
#include "LocalSearch.h"

#include "Cidade.hpp"

void
LocalSearch::melhoria_2_opt(Particle &p){
    int begin = 0;
    int desalinhamento = 0;

    auto solucao = p.get_full_solution();

    for(int i = 1; i < solucao.size(); i++){
        if(solucao[i] == 0){
            melhorar_rota(solucao, p, begin, i, desalinhamento);
            begin = i;
            desalinhamento++;
        }
    }
    p.atualiza_dist(cidades, capacidadeV);

}

void
LocalSearch::melhorar_rota(vector<int> &rota, Particle &p, int begin, int end, int des){
    bool improved = true;
    // cout << "sub-rota inicial: " << calcula_caminho(rota, begin, end) << endl;
    // for(int i = begin; i < end; i++){
    //     cout << rota[i] << "-";
    // }
    auto &c = cidades;
    while (improved) {
        improved = false;
        for (size_t i = begin + 1; i < end; ++i) {
            for (size_t j = i + 1; j < end; ++j) {
                // Calcula a distância antes da troca
                double old_dist = Cidade::distancia(c[rota[i-1]], c[rota[i]]) + Cidade::distancia(c[rota[j]], c[rota[(j+1)]]);
                // Distância após a troca (2-opt swap)
                double new_dist = Cidade::distancia(c[rota[i-1]], c[rota[j]]) + Cidade::distancia(c[rota[i]], c[rota[(j+1)]]);
                // cout << "dist: " << old_dist << "/" << new_dist << endl;
                // Se a nova distância for menor, aplica a troca
                if (new_dist < old_dist) {

                    std::reverse(rota.begin() + i, rota.begin() + j + 1);

                    improved = true;
                }
            }
        }
    }
    for(int i = begin + 1; i < end; i++){
        p.solucao_atual[i - des] = rota[i];
    }
    // cout << "sub-rota otimizada: " << calcula_caminho(rota, begin, end) << endl;
    // cout << "sub-rota inicial: " << calcula_caminho(rota, begin, end) << endl;
    // for(int i = begin; i < end; i++){
    //     cout << rota[i] << "-";
    // }
    // cout << endl << endl;
}

void
LocalSearch::swap_star(Particle &p) {
    vector<pair<unsigned, unsigned>> limites_rotas;

    auto solucao = p.get_full_solution(limites_rotas);

    for (unsigned i = 0; i < limites_rotas.size() - 1; ++i) {
        for (unsigned j = i + 1; j < limites_rotas.size(); ++j) {
            unordered_map<unsigned, vector<insert_info>> top3_insert_v;
            unordered_map<unsigned, vector<insert_info>> top3_insert_u;

            // Pré-processar melhores custos de inserção:
            for (unsigned id_v = limites_rotas[i].first + 1; id_v < limites_rotas[i].second; ++id_v)
                top3_insert_v[id_v] = findTop3Locations(solucao, id_v, limites_rotas[j]);

            for (unsigned id_u = limites_rotas[j].first + 1; id_u < limites_rotas[j].second; ++id_u)
                top3_insert_u[id_u] = findTop3Locations(solucao, id_u, limites_rotas[i]);

            double best_swap = 0;

            insert_info best_v;
            insert_info best_u;

            // Encontrar melhor par de clientes para trocar entre duas rotas, assim como a posição deles nelas

            for (unsigned id_v = limites_rotas[i].first + 1; id_v < limites_rotas[i].second; ++id_v) {
                for (unsigned id_u = limites_rotas[j].first + 1; id_u < limites_rotas[j].second; ++id_u) {

                    // Melhor inserção de v em r', fora inserção no mesmo lugar que o U e desconsiderando este da rota
                        auto k = min_element(top3_insert_v[id_v].begin(), top3_insert_v[id_v].end(),
                            [&id_u](insert_info& a, insert_info& b) {
                                return (a.cost < b.cost) && (a.pred != id_u && a.suce != id_u);
                            });

                    // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
                    auto k_ln = min_element(top3_insert_u[id_u].begin(), top3_insert_u[id_u].end(),
                        [&id_v](insert_info& a, insert_info& b) {
                            return (a.cost < b.cost) && (a.pred != id_v && a.suce != id_v);
                        });

                    // Melhor custo de inserção de V em r'
                    double swap_v_em_u = insertion_cost(solucao, id_v, id_u-1, id_u+1);
                    double v_para_r_ln = min(swap_v_em_u, k->cost) - insertion_cost(solucao, id_v, id_v-1, id_v+1);

                    // Melhor custo de inserção de U em r
                    double swap_u_em_v = insertion_cost(solucao, id_u, id_v-1, id_v+1);
                    double u_para_r = min(swap_u_em_v, k_ln->cost)
                    - insertion_cost(solucao, id_u, id_u-1, id_u+1);

                    // Atualizar a melhor troca encontrada
                    if (auto c = v_para_r_ln + u_para_r; c < best_swap) {
                        best_swap = c;
                        best_v = swap_v_em_u < k->cost ? insert_info{id_v, id_u-1, id_u+1} : *k;
                        best_u = swap_u_em_v < k_ln->cost ? insert_info{id_u, id_v-1, id_v+1} : *k_ln;
                    }
                }
            }
            // A melhor troca que otimiza a solução, caso exista, é executada
            if (best_swap < 0) {
                cout << "Rotas: " << i << " e "<< j << endl;
                cout << "índices sol: " << best_v.id << " e " << best_u.id << endl;
                cout << "clientes: " << solucao[best_v.id] << " e " << solucao[best_u.id] << endl;
                cout << "pred e suc v: " << solucao[best_v.pred] << " e " << solucao[best_v.suce] << endl;
                cout << "pred e suc u: " << solucao[best_u.pred] << " e " << solucao[best_u.suce] << endl;
                cout << endl;
                apply_swap(p, solucao, best_swap, i, j, best_v, best_u);
                auto a = 2;
            }

        // cout << "Checou a rota com uma da frente " << i << ", "<< j << endl;
        }
        // cout << "Checou a rota com todas as da frente "  << i << endl;
    }
}

vector<LocalSearch::insert_info>
LocalSearch::findTop3Locations(vector<int>& sol, unsigned id_v, const pair<unsigned, unsigned> &limiters) {

    vector<insert_info> top3;
    auto cmp = [](insert_info& a, insert_info& b) {
        return a.cost < b.cost;
    };

    for (unsigned i = limiters.first; i < limiters.second; ++i) {
        if (top3.empty()) {
            top3.push_back(insert_info{id_v, i, i+1, insertion_cost(sol[id_v], sol[i], sol[i+1])});
        }
        else if (top3.size() < 3) {
            top3.push_back(insert_info{id_v, i, i+1, insertion_cost(sol[id_v], sol[i], sol[i+1])});
            std::sort(top3.begin(), top3.end(), cmp);
        }
        else if (auto c = insertion_cost(sol[id_v], sol[i], sol[i+1]); c < top3.back().cost) {
            top3.back() = insert_info{id_v, i, i+1, c};
            std::sort(top3.begin(), top3.end(), cmp);
        }
    }

    return top3;
}

void
LocalSearch::apply_swap(Particle& p, vector<int>& sol, double cost, int des_v, int des_u, insert_info &v, insert_info &u) {

    unsigned dest_v, dest_u;
    int valor_v = p.solucao_atual[v.id - des_v];
    int valor_u = p.solucao_atual[u.id - des_u];

    if (v.pred < u.id) {
        dest_v = v.pred + 1 - des_u;
        for (int i = u.id - des_u; i > dest_v; --i) {
            sol[i + des_u] = sol[i+des_u -1];
            p.solucao_atual[i] = p.solucao_atual[i-1];
        }
    }
    else if (v.pred > u.id) {
        dest_v = v.pred - des_u;
        for (int i = u.id - des_u; i < dest_v; ++i) {
            sol[i + des_u] = sol[i+des_u +1];
            p.solucao_atual[i] = p.solucao_atual[i+1];
        }
    }
    else {
        dest_v = u.id - des_u;
    }
    p.solucao_atual[dest_v] = valor_v;
    sol[dest_v+des_u] = valor_v;

    if (u.pred < v.id) {
        dest_u = u.pred + 1 - des_v;
        for (int i = v.id - des_v; i > dest_u; --i) {
            sol[i+des_v] = sol[i+des_v -1];
            p.solucao_atual[i] = p.solucao_atual[i-1];
        }
    }
    else if (u.pred > v.id) {
        dest_u = u.pred - des_v;
        for (int i = v.id - des_v; i < dest_u; ++i) {
            sol[i+des_v] = sol[i+des_v +1];
            p.solucao_atual[i] = p.solucao_atual[i+1];
        }
    }
    else {
        dest_u = v.id - des_v;
    }
    p.solucao_atual[dest_u] = valor_u;
    sol[dest_u + des_v] = valor_u;

    p.atualiza_dist(p.dist_atual + cost);
}
