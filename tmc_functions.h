#ifndef __TMC_FUNCTIONS_H
#define __TMC_FUNCTIONS_H

#include "tmc_utility.h"
#include "tmc_process.h"
#include "tmc_tmcar.h"

struct compare_bfs {
  bool operator () (const std::pair<double, std::vector<TM_OTH::point_in_simulation>> a,
                   const std::pair<double, std::vector<TM_OTH::point_in_simulation>> b) {
    if (!TM_OTH::unwanted_cmp<double>(a.first, b.first))
      return a.first > b.first;

    return (int)a.second.size() < (int)b.second.size();
  }
};

void teleport_car_by_ox (TM_CAR &tmc, PROCESS_T &proc);

void get_pos_indefinitely(TM_CAR &tmc, PROCESS_T &proc);

void move_car_by_ox (TM_CAR &tmc, PROCESS_T &proc);

void checkpoint_reach (TM_CAR &tmc, PROCESS_T &proc);

void test_race_restart_upon_ending (TM_CAR &tmc, PROCESS_T &proc);

/**
  sim_points: runs through the provided points.
  run_until: stop the simulation when timer goes over it.
  returns the score of the simulation.
**/
std::pair<double, bool>
run_simulation (TM_CAR &tmc, PROCESS_T &proc,
                std::vector<TM_OTH::point_in_simulation> &sim_points, int run_until);

void breadth_first_search (TM_CAR &tmc, PROCESS_T &proc);

void depth_first_search (TM_CAR &tmc, PROCESS_T &proc, std::vector<TM_OTH::point_in_simulation> &sim_points);

#endif
