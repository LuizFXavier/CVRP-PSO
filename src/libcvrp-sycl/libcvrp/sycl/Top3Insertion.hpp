#pragma once

#include <libcvrp/core/constants.hpp>

namespace cvrp::sycl_engine
{

struct insert_info
{
  int insert_index{};
  int client_pred{};
  int client_suce{};
  float cost{};
};

struct BestSwap
{
  float total_cost;
  int v_tour_id;
  int u_tour_id;
  int v_tour_dest;
  int u_tour_dest;
  int r_i;
  int r_j;
  // int best_v_id;
  // int best_u_id;
  // insert_info best_v;
  // insert_info best_u;
};

struct FindBestSwap
{
  BestSwap 
  operator()(const BestSwap& a, const BestSwap& b) const 
  {
    return (a.total_cost < b.total_cost) ? a : b;
  }
};

struct Top3Insertion
{
  insert_info best_insertions[3];
  
  void 
  compareAndAdd(insert_info candidate)
  {
    if (candidate.cost >= best_insertions[2].cost) 
      return;

    else if (candidate.cost >= best_insertions[1].cost){

      best_insertions[2] = candidate;
    }
    else if (candidate.cost >= best_insertions[0].cost){

      best_insertions[2] = best_insertions[1];
      best_insertions[1] = candidate;
    }
    else{
      best_insertions[2] = best_insertions[1];
      best_insertions[1] = best_insertions[0];
      best_insertions[0] = candidate;
    }

  }
  insert_info
  get_best_insertion_except_client(int client){

    if (best_insertions[0].client_pred != client && best_insertions[0].client_suce != client)
      return best_insertions[0];

    if (best_insertions[1].client_pred != client && best_insertions[1].client_suce != client)
      return best_insertions[1];

    if (best_insertions[2].client_pred != client && best_insertions[2].client_suce != client)
      return best_insertions[2];

    return {0, 0, 0, cvrp::INF_F};
  }
  Top3Insertion(){
    best_insertions[0].cost = cvrp::INF_F;
    best_insertions[1].cost = cvrp::INF_F;
    best_insertions[2].cost = cvrp::INF_F;
  }
};
} // namespace cvrp::sycl_engine
