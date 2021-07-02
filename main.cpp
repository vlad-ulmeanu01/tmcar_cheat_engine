#include "tmc_utility.h"
#include "tmc_process.h"
#include "tmc_tmcar.h"
#include "tmc_functions.h"

int main() {
  TM_CAR tmc;

  ///tmc.parse_input_file_to_umap<float>("tm_values.txt", tmc.values);
  tmc.parse_input_file_to_umap<DWORD>("TM_DATA/tm_value_addresses.txt", tmc.value_addresses);
  tmc.parse_points_of_interest("TM_DATA/poi_tastrain01.txt");
  tmc.parse_other_data("TM_DATA/oth_tastrain01.txt");
  tmc.init_keymapping();

  PROCESS_T proc;
  proc.get_process_by_name("TrackMania United Forever");

  ///--TESTING--
  //teleport_car_by_ox(tmc, proc);
  //get_pos_indefinitely(tmc, proc);
  //move_car_by_ox(tmc, proc);
  //checkpoint_reach(tmc, proc);
  test_race_restart_upon_ending(tmc, proc);

  ///--ACTUAL CODE--
//  std::vector<TM_OTH::point_in_simulation> sim_points;
//
//  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
//  depth_first_search(tmc, proc, sim_points);

  return 0;
}
