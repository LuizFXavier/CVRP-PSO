#include <algorithm>
#include <unordered_map>
#include <iostream>
#include "LocalSearch.h"

#include "Cidade.hpp"

void 
LocalSearch::search(Particle &p)
{
    auto rotas = p.get_routes();

    // for (auto r : rotas)
    // {
    //     cout << r.custo << ", real: " << route_cost(r) << endl;
    // }
    //
    // double preco = 0, preco_r = 0;
    // for(auto r : rotas){
    //     for(auto i : r.path)
    //         std::cout << i << ", ";
    //     preco += r.custo;
    // }

    // cout << endl << "Custo inicial:" << preco << endl;
    // preco = 0;
    two_opt(rotas);
    // cout << endl << "Pós 2-opt:" << preco << endl;

    swap_star(rotas);
    //
    // two_opt(rotas);

    // for(auto r : rotas){
    //     unsigned demand = 0;
    //     cout << "[";
    //     for(auto i : r.path)
    //     {
    //         std::cout << i << " ";
    //         demand += cidades[i].demanda;
    //     }
    //     cout << "] " << demand << ", real:" << route_cost(r) << ", obt:" << r.custo << endl;
    //     demand = 0;
    //     preco += r.custo;
    //     preco_r += route_cost(r);
    // }


    p.update_tour(rotas);
}

void 
LocalSearch::two_opt(vector<Route> &r)
{   
    bool improved = true;
    auto &c = cidades;

    for(auto& rota : r)
        while (improved)
        {
            improved = false;
            for(size_t i = 1; i < rota.size()-1; ++i)
                for (size_t j = i+1; j < rota.size()-1; ++j)
                {
                    // Calcula a distância antes da troca
                    double old_dist = Cidade::distancia(c[rota[i-1]], c[rota[i]]) + Cidade::distancia(c[rota[j]], c[rota[(j+1)]]);

                    // Distância após a troca (2-opt swap)
                    double new_dist = Cidade::distancia(c[rota[i-1]], c[rota[j]]) + Cidade::distancia(c[rota[i]], c[rota[(j+1)]]);
                    
                    // Se a nova distância for menor, aplica a troca
                    if (new_dist < old_dist) {

                        std::reverse(rota.path.begin() + i, rota.path.begin() + j + 1);

                        rota.custo = rota.custo - old_dist + new_dist;
                        improved = true;
                    }
                }
                
        }
        
}

void LocalSearch::swap_star(vector<Route> &r)
{
    for(unsigned i = 0; i < r.size()-1; ++i){
        for(unsigned j = i+1; j < r.size(); ++j){
            unordered_map<unsigned, vector<insert_info>> top3_insert_v;
            unordered_map<unsigned, vector<insert_info>> top3_insert_u;

            // Pré-processar melhores custos de inserção:
            for (unsigned id_v = 1; id_v < r[i].size()-1; ++id_v)
                top3_insert_v[id_v] = findTop3Locations(r[i][id_v], r[j]);

            for (unsigned id_u = 1; id_u < r[j].size()-1; ++id_u)
                top3_insert_u[id_u] = findTop3Locations(r[j][id_u], r[i]);

            double best_swap_cost = 0;

            insert_info best_v;
            insert_info best_u;

            unsigned best_v_id, best_u_id;

            for (unsigned id_v = 1; id_v < r[i].size()-1; ++id_v){
                for (unsigned id_u = 1; id_u < r[j].size()-1; ++id_u){
                    
                    unsigned v = r[i][id_v], u = r[j][id_u];

                    if(r[i].demanda - cidades[v].demanda + cidades[u].demanda > capacidadeV)
                        continue;
                    
                    if(r[j].demanda - cidades[u].demanda + cidades[v].demanda > capacidadeV)
                        continue;
                    
                    // Melhor inserção de v em r', fora inserção no mesmo lugar que o U e desconsiderando este da rota
                    auto k = min_element(top3_insert_v[id_v].begin(), top3_insert_v[id_v].end(),
                    [&id_u](insert_info& a, insert_info& b) {
                        if (a.pred == id_u || a.suce == id_u) return false;
                        if (b.pred == id_u || b.suce == id_u) return true;
                        return (a.cost < b.cost);
                    });

                    k->cost = (k->suce != id_u && k->pred != id_u) ? k->cost : INFINITO;

                    // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
                    auto k_ln = min_element(top3_insert_u[id_u].begin(), top3_insert_u[id_u].end(),
                        [&id_v](insert_info& a, insert_info& b) {
                            if (a.pred == id_v || a.suce == id_v) return false;
                            if (b.pred == id_v || b.suce == id_v) return true;
                            return (a.cost < b.cost);
                        });



                    k_ln->cost = (k_ln->suce != id_v && k_ln->pred != id_v) ? k_ln->cost : INFINITO;

                    double swap_v_em_u = insertion_cost(r[i][id_v], r[j][id_u -1], r[j][id_u+1]);

                    // Melhor custo de inserção de V em r'
                    double v_para_r_ln = min(swap_v_em_u, k->cost)
                    - insertion_cost(r[i][id_v], r[i][id_v-1], r[i][id_v+1]);

                    // Melhor custo de inserção de U em r
                    double swap_u_em_v = insertion_cost(r[j][id_u], r[i][id_v -1], r[i][id_v+1]);

                    
                    double u_para_r = min(swap_u_em_v, k_ln->cost)
                    - insertion_cost(r[j][id_u], r[j][id_u-1], r[j][id_u+1]);

                    // Atualizar a melhor troca encontrada
                    if (auto c = v_para_r_ln + u_para_r; c < best_swap_cost) {

                        best_swap_cost = c;
                        best_v = swap_v_em_u < k->cost ? insert_info{id_u-1, id_u+1, swap_v_em_u} : *k;
                        best_v_id = id_v;

                        best_u = swap_u_em_v < k_ln->cost ? insert_info{id_v-1, id_v+1, swap_u_em_v} : *k_ln;
                        best_u_id = id_u;

                        auto rm_v = insertion_cost(r[i][id_v], r[i][id_v -1], r[i][id_v+1]);
                        auto rm_u = insertion_cost(r[j][id_u], r[j][id_u -1], r[j][id_u+1]);
                        // cout << "swap_v_em_u: " << swap_v_em_u << " k->cost: " << k->cost << endl;
                        // cout << "swap_u_em_v: " << swap_u_em_v << " k_ln->cost: " << k_ln->cost << endl;
                        // cout << "best_v.cost: " << best_v.cost << endl;
                        // cout << "best_u.cost: " << best_u.cost << endl;
                        // cout << "rm_v: " << rm_v << " rm_u: " << rm_u << endl;
                        // cout << "v: " << v << " , u: " << u << endl;
                        // cout << "best_swap_cost: " << best_swap_cost << endl << endl;
                    }
                }
            }
            // A melhor troca que otimiza a solução, caso exista, é executada
            if (best_swap_cost < 0) {
                // cout << "Best swap cost:" << best_swap_cost << endl;
                // cout << "[";
                // for (auto i : r[i].path)
                //     cout << i << " ";
                // cout << "] [";
                // for (auto i : r[j].path)
                //     cout << i << " ";
                // cout << "]" << endl;
                // cout << "i, j:" << i << ", " << j << endl << endl;

                apply_swap(best_v_id, best_u_id, r[i], r[j], best_v, best_u);
            }
        }
    }
}

void 
LocalSearch::apply_swap(unsigned id_v, unsigned id_u, Route &r, Route &r_ln, insert_info& v_insert, insert_info& u_insert)
{
    unsigned dest_v, dest_u;
    unsigned v = r[id_v];
    unsigned u = r_ln[id_u];

    double rm_v = insertion_cost(v, r[id_v-1], r[id_v+1]);
    double rm_u = insertion_cost(u, r_ln[id_u-1], r_ln[id_u+1]);


    // cout << "v: " << v << ", u: " << u << endl;

    // cout << "ant cost R: " << r.custo << " R_ln: " << r_ln.custo << endl;
    //
    // cout << "[";
    // for (auto i : r.path)
    //     cout << i << " ";
    // cout << "] [";
    // for (auto i : r_ln.path)
    //     cout << i << " ";
    // cout << "]" << endl;

    // cout << "Insert v: (" << r_ln[v_insert.pred] << ", " << r_ln[v_insert.suce] << ", " << v_insert.cost << ")" << endl;
    //
    // cout << "Insert u: (" << r[u_insert.pred] << ", " << r[u_insert.suce] << ", " << u_insert.cost << ")" << endl;

    auto dummy_r = r.custo;
    auto dummy_l = r_ln.custo;

    r.custo = r.custo - rm_v;// + u_insert.cost;
    r_ln.custo = r_ln.custo - rm_u;// + v_insert.cost;

    r.custo += u_insert.cost;
    r_ln.custo += v_insert.cost;

    insertion_cost(v, r[id_v-1], r[id_v+1], r.custo + u_insert.cost);
    insertion_cost(u, r_ln[id_u-1], r_ln[id_u+1], r_ln.custo + v_insert.cost);

    r.demanda = r.demanda - cidades[v].demanda + cidades[u].demanda;
    r_ln.demanda = r_ln.demanda - cidades[u].demanda + cidades[v].demanda;

    if(v_insert.pred < id_u){
        dest_v = v_insert.pred + 1;
        for(int i = id_u; i > dest_v; --i){
            r_ln[i] = r_ln[i-1];
        }
    }
    else if(v_insert.pred > id_u){
        dest_v = v_insert.pred;
        for (int i = id_u; i < dest_v; ++i) {
            r_ln[i] = r_ln[i+1];
        }
    }
    else{
        dest_v = id_u;
    }

    if(u_insert.pred < id_v){
        dest_u = u_insert.pred + 1;
        for(int i = id_v; i > dest_u; --i){
            r[i] = r[i-1];
        }
    }
    else if(u_insert.pred > id_v){
        dest_u = u_insert.pred;
        for (int i = id_v; i < dest_u; ++i) {
            r[i] = r[i+1];
        }
    }
    else{
        dest_u = id_v;
    }

    r[dest_u] = u;
    r_ln[dest_v] = v;

    // cout << "[";
    // for (auto i : r.path)
    //     cout << i << " ";
    // cout << "] [";
    // for (auto i : r_ln.path)
    //     cout << i << " ";
    // cout << "]" << endl << endl;
    //
    // cout << "real cost R: " << route_cost(r) << " R_ln: " << route_cost(r_ln) << endl;
}

auto
LocalSearch::findTop3Locations(unsigned v, Route &r_ln)->vector<insert_info>
{   
    vector<insert_info> top3;

    auto cmp = [](insert_info& a, insert_info& b) {
        return a.cost < b.cost;
    };
    
    for(unsigned i = 0; i < r_ln.size() - 1; ++i){
        if(top3.empty()){
            top3.emplace_back(insert_info{i, i+1, insertion_cost(v, r_ln[i], r_ln[i+1])});
        }
        else if (top3.size() < 3) {
            top3.push_back(insert_info{i, i+1, insertion_cost(v, r_ln[i], r_ln[i+1])});
            std::sort(top3.begin(), top3.end(), cmp);
        }
        else if (auto c = insertion_cost(v, r_ln[i], r_ln[i+1]); c < top3.back().cost) {
            top3.back() = insert_info{i, i+1, c};
            std::sort(top3.begin(), top3.end(), cmp);
        }

    }

    return top3;
}

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
LocalSearch::swap_star_legacy(Particle &p) {
    vector<pair<unsigned, unsigned>> limites_rotas;

    auto solucao = p.get_full_solution(limites_rotas);

    for (unsigned i = 0; i < limites_rotas.size() - 1; ++i) {
        for (unsigned j = i + 1; j < limites_rotas.size(); ++j) {
            unordered_map<unsigned, vector<insert_info_legacy>> top3_insert_v;
            unordered_map<unsigned, vector<insert_info_legacy>> top3_insert_u;

            // Pré-processar melhores custos de inserção:
            for (unsigned id_v = limites_rotas[i].first + 1; id_v < limites_rotas[i].second; ++id_v)
                top3_insert_v[id_v] = findTop3Locations(solucao, id_v, limites_rotas[j]);

            for (unsigned id_u = limites_rotas[j].first + 1; id_u < limites_rotas[j].second; ++id_u)
                top3_insert_u[id_u] = findTop3Locations(solucao, id_u, limites_rotas[i]);

            double best_swap = 0;

            insert_info_legacy best_v;
            insert_info_legacy best_u;

            // Encontrar melhor par de clientes para trocar entre duas rotas, assim como a posição deles nelas

            for (unsigned id_v = limites_rotas[i].first + 1; id_v < limites_rotas[i].second; ++id_v) {
                for (unsigned id_u = limites_rotas[j].first + 1; id_u < limites_rotas[j].second; ++id_u) {

                    // Melhor inserção de v em r', fora inserção no mesmo lugar que o U e desconsiderando este da rota
                        auto k = min_element(top3_insert_v[id_v].begin(), top3_insert_v[id_v].end(),
                            [&id_u](insert_info_legacy& a, insert_info_legacy& b) {
                                return (a.cost < b.cost) && (a.pred != id_u && a.suce != id_u);
                            });

                    // Melhor inserção de u em r, fora inserção no mesmo lugar que o V e desconsiderando este da rota
                    auto k_ln = min_element(top3_insert_u[id_u].begin(), top3_insert_u[id_u].end(),
                        [&id_v](insert_info_legacy& a, insert_info_legacy& b) {
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
                        best_v = swap_v_em_u < k->cost ? insert_info_legacy{id_v, id_u-1, id_u+1} : *k;
                        best_u = swap_u_em_v < k_ln->cost ? insert_info_legacy{id_u, id_v-1, id_v+1} : *k_ln;
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

vector<LocalSearch::insert_info_legacy>
LocalSearch::findTop3Locations(vector<int>& sol, unsigned id_v, const pair<unsigned, unsigned> &limiters) {

    vector<insert_info_legacy> top3;
    auto cmp = [](insert_info_legacy& a, insert_info_legacy& b) {
        return a.cost < b.cost;
    };

    for (unsigned i = limiters.first; i < limiters.second; ++i) {
        if (top3.empty()) {
            top3.push_back(insert_info_legacy{id_v, i, i+1, insertion_cost(sol[id_v], sol[i], sol[i+1])});
        }
        else if (top3.size() < 3) {
            top3.push_back(insert_info_legacy{id_v, i, i+1, insertion_cost(sol[id_v], sol[i], sol[i+1])});
            std::sort(top3.begin(), top3.end(), cmp);
        }
        else if (auto c = insertion_cost(sol[id_v], sol[i], sol[i+1]); c < top3.back().cost) {
            top3.back() = insert_info_legacy{id_v, i, i+1, c};
            std::sort(top3.begin(), top3.end(), cmp);
        }
    }

    return top3;
}

void LocalSearch::apply_swap(Particle &p, vector<int> &sol, double cost, int des_v, int des_u, insert_info_legacy &v, insert_info_legacy &u)
{

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
