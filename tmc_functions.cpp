#include "tmc_utility.h"
#include "tmc_process.h"
#include "tmc_tmcar.h"
#include "tmc_functions.h"

void teleport_car_by_ox (TM_CAR &tmc, PROCESS_T &proc) {
  tmc.values["pos_x"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]);

  for (int i = 0; i < 30; i++) {
    tmc.values["pos_x"]++;
    proc.write_value_to_address<float>(tmc.value_addresses["pos_x"], tmc.values["pos_x"]);
  }
}

void get_pos_indefinitely(TM_CAR &tmc, PROCESS_T &proc) {
  while (1) {
    tmc.update_from_memory(proc);

    std::cout << tmc.values["pos_x"] << ' ';
    std::cout << tmc.values["pos_y"] << ' ';
    std::cout << tmc.values["pos_z"] << ' ';
    std::cout << tmc.values["timer"] << ' ';
    std::cout << tmc.values["ckpts"] << '\n';

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void move_car_by_ox (TM_CAR &tmc, PROCESS_T &proc) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  tmc.restart_race();

  tmc.values["pos_x"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]);
  float target_x = tmc.values["pos_x"] + 30;

  while (proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]) < target_x) {
    tmc.tap_key('i');
  }
}

void checkpoint_reach (TM_CAR &tmc, PROCESS_T &proc) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  tmc.restart_race();

  while (1) {
    tmc.update_from_memory(proc);
    tmc.update_points_of_interest();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

/**
  sim_points: runs through the provided points.
  run_until: stop the simulation when timer goes over it.
  returns the score of the simulation.
**/
double run_simulation (TM_CAR &tmc, PROCESS_T &proc,
                     std::vector<TM_OTH::point_in_simulation> &sim_points, int run_until) {
  tmc.restart_race();
  tmc.update_from_memory(proc);

  int current_index = 0;
  while (current_index < (int)sim_points.size() && tmc.values["timer"] < run_until) {
    while (current_index + 1 < (int)sim_points.size() &&
           tmc.values["timer"] >= sim_points[current_index + 1].run_after_this_time)
      current_index++;

    /// now I have to work with sim_points[current_index]
    tmc.update_points_of_interest();
    tmc.tap_keys(sim_points[current_index]);

    tmc.update_from_memory(proc);
  }

  std::cout << "simulation score: " << tmc.fitness_function() << '\n';

  return tmc.fitness_function();
}

void depth_first_search (TM_CAR &tmc, PROCESS_T &proc, std::vector<TM_OTH::point_in_simulation> &sim_points) {
  if ((int)sim_points.size() > 4) {
    run_simulation(tmc, proc, sim_points, 6000);
    return;
  }

  int next_timestamp = 0;
  if ((int)sim_points.size() > 0)
    next_timestamp = 500 + sim_points.back().run_after_this_time;

  sim_points.push_back(TM_OTH::point_in_simulation());
  sim_points.back().run_after_this_time = next_timestamp;

  sim_points.back().is_pressed["up"] = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_pressed["up"] = false;

  sim_points.back().is_pressed["up"] = true;
  sim_points.back().is_pressed["ri"] = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_pressed["up"] = false;
  sim_points.back().is_pressed["ri"] = false;

  sim_points.back().is_pressed["up"] = true;
  sim_points.back().is_pressed["le"] = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_pressed["up"] = false;
  sim_points.back().is_pressed["le"] = false;

  /// TODO pune mai multe bobite pe circuit

//  sim_points.back().is_pressed["ri"] = true;
//  depth_first_search(tmc, proc, sim_points);
//  sim_points.back().is_pressed["ri"] = false;

  //depth_first_search(tmc, proc, sim_points);
  /// ^ nu are sens sa nu faci nimic daca tot dai tap

  sim_points.pop_back();
}

