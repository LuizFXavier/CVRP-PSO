#pragma once

#include <libcvrp/core/constants.hpp>

namespace cvrp::sycl_engine
{

struct insert_info
{
  int pred{};
  int suce{};
  float cost{};
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
  get_best_insertion_except_id(unsigned id){

    if (best_insertions[0].pred != id && best_insertions[0].suce != id)
      return best_insertions[0];

    if (best_insertions[1].pred != id && best_insertions[1].suce != id)
      return best_insertions[1];

    if (best_insertions[2].pred != id && best_insertions[2].suce != id)
      return best_insertions[2];

    return {0, 0, cvrp::INF_F};
  }
  Top3Insertion(){
    best_insertions[0].cost = cvrp::INF_F;
    best_insertions[1].cost = cvrp::INF_F;
    best_insertions[2].cost = cvrp::INF_F;
  }
};
} // namespace cvrp::sycl_engine
