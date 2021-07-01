#ifndef __TMC_FUNCTIONS_H
#define __TMC_FUNCTIONS_H

#include "tmc_utility.h"
#include "tmc_process.h"
#include "tmc_tmcar.h"

void teleport_car_by_ox (TM_CAR &tmc, PROCESS_T &proc);

void get_pos_indefinitely(TM_CAR &tmc, PROCESS_T &proc);

void move_car_by_ox (TM_CAR &tmc, PROCESS_T &proc);

void checkpoint_reach (TM_CAR &tmc, PROCESS_T &proc);
/**
  sim_points: runs through the provided points.
  run_until: stop the simulation when timer goes over it.
  returns the score of the simulation.
**/
double run_simulation (TM_CAR &tmc, PROCESS_T &proc,
                     std::vector<TM_OTH::point_in_simulation> &sim_points, int run_until);

void depth_first_search (TM_CAR &tmc, PROCESS_T &proc, std::vector<TM_OTH::point_in_simulation> &sim_points);

#endif
