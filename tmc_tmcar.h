#ifndef __TMC_TMCAR_H
#define __TMC_TMCAR_H

#include "tmc_utility.h"
#include "tmc_process.h"

class TM_CAR {
private:
  DWORD interpret_string_into_hex (std::string s);
public:
  /**
    the dictionaries accept:
    pos_x, pos_y, pos_z, timer, ckpts, speed, distl
  **/
  std::unordered_map<std::string, float> values;  /// values for accepted strings
  std::unordered_map<std::string, DWORD> value_addresses;  /// values for addreses of accepted strings
  /**
  * opens filename, clears the write_to umap (which has to be one of TM_CAR's), then writes all
    entries from the opened file into the umap.
  * the file's entries must be written in the following manner:
    pos_y 0x54584589
    vel_z 0x43838984

    or
    vel_x 34.32
    pos_z 439
    ...
  **/

  /// points of interest for the circuit.
  std::vector<TM_OTH::point_of_interest> points_of_interest;

  /// the mapping for the keys for this car.
  std::unordered_map<std::string, char> key_mapping;

  /// total number of checkpoints on map. used to verify if run has ended after finish.
  int total_no_checkpoints;

  int ss_size_cutoff_point;

  int ss_size_cutoff_cut_to;

  int bfs_window_time_ms;

  int bfs_add_time_ms;


  void init_keymapping ();

  template<typename T>
  void parse_input_file_to_umap(std::string filename, std::unordered_map<std::string, T> &write_to);

  void parse_points_of_interest(std::string filename);

  void parse_other_data (std::string filename);

  void update_from_memory (PROCESS_T &proc);

  /// returns true if at least one POI has been passed through since last time.
  bool update_points_of_interest ();

  double fitness_function ();

  void restart_race (PROCESS_T &proc);

  void tap_key (char ch);

  /// will tap simultaneously any combination of the up/dn/le/ri keys.
  void tap_keys (TM_OTH::point_in_simulation point);
};

#endif
