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
    std::cout << tmc.values["ckpts"] << ' ';
    std::cout << tmc.values["speed"] << ' ';
    std::cout << tmc.values["distl"] << '\n';

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void move_car_by_ox (TM_CAR &tmc, PROCESS_T &proc) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  tmc.restart_race(proc);

  tmc.values["pos_x"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]);
  float target_x = tmc.values["pos_x"] + 30;

  while (proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]) < target_x) {
    tmc.tap_key('i');
  }
}

void checkpoint_reach (TM_CAR &tmc, PROCESS_T &proc) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  tmc.restart_race(proc);

  while (1) {
    tmc.update_from_memory(proc);
    tmc.update_points_of_interest();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

void test_race_restart_upon_ending (TM_CAR &tmc, PROCESS_T &proc) {
  tmc.update_from_memory(proc);

  while (tmc.values["ckpts"] < tmc.total_no_checkpoints) {
    tmc.update_from_memory(proc);
    tmc.update_points_of_interest();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  tmc.restart_race(proc);
}

void replay_file (TM_CAR &tmc, PROCESS_T &proc, std::string filename) {
  std::vector<TM_OTH::point_in_simulation> sim_points;
  TM_OTH::get_simulation(filename, sim_points);

  double fit;
  bool has_ended;
  std::tie(fit, has_ended) = run_simulation(tmc, proc, sim_points, 20000);

  std::cout << filename << ":\n";
  std::cout << "fitness score: " << fit << '\n';
  std::cout << "has ended: " << has_ended << '\n';
}

/**
  sim_points: runs through the provided points.
  run_until: stop the simulation when timer goes over it.
  returns the score of the simulation (first) and if the race crossed the finish line (second).
**/
std::pair<double, bool>
run_simulation(TM_CAR &tmc, PROCESS_T &proc,
               std::vector<TM_OTH::point_in_simulation> &sim_points, int run_until)
{
  std::cout << "sim_points size: " << (int)sim_points.size() << '\n';
  for (auto x: sim_points) {
    if (x.is_pressed["up"] && !x.is_pressed["le"] && !x.is_pressed["ri"]) std::cout << "A";
    if (x.is_pressed["up"] && !x.is_pressed["le"] && x.is_pressed["ri"]) std::cout << "B";
    if (x.is_pressed["up"] && x.is_pressed["le"] && !x.is_pressed["ri"]) std::cout << "C";
  }
  std::cout << '\n';


  tmc.restart_race(proc);
  tmc.update_from_memory(proc);

  int current_index = 0, supervisor_opinion = 0;
  ///if supervisior_opinion == 1 <=> good run, -1 <=> bad run, should discard, 0 <=> no input from supervisor

  while (current_index < (int)sim_points.size() &&
         tmc.values["timer"] < run_until &&
         tmc.values["ckpts"] < tmc.total_no_checkpoints)
  {
    while (current_index + 1 < (int)sim_points.size() &&
           tmc.values["timer"] >= sim_points[current_index + 1].run_after_this_time)
      current_index++;

    /// now I have to work with sim_points[current_index]
    tmc.update_points_of_interest();
    tmc.tap_keys(sim_points[current_index]);

    tmc.update_from_memory(proc);

    if (tmc.is_key_pressed(tmc.key_mapping["like"]))
      supervisor_opinion = 1;
    if (tmc.is_key_pressed(tmc.key_mapping["dislike"]))
      supervisor_opinion = -1;
    if (tmc.is_key_pressed(tmc.key_mapping["neutral"]))
      supervisor_opinion = 0;
  }

  if (TM_OTH::rational_cmp<float>(tmc.values["ckpts"], tmc.total_no_checkpoints))
    TM_OTH::store_simulation(sim_points);

  double fit = tmc.fitness_function();
  if (supervisor_opinion == 1) {
    fit = fabs(fit) * 10;
    std::cout << "LIKED THIS RUN.\n";
  } else if (supervisor_opinion == -1) {
    fit = -9000.0;
    std::cout << "DISLIKED THIS RUN.\n";
  }

  std::cout << "simulation score: " << fit << '\n';

  return std::make_pair(fit, TM_OTH::rational_cmp<float>(tmc.values["ckpts"], tmc.total_no_checkpoints));
}

void breadth_first_search (TM_CAR &tmc, PROCESS_T &proc) {
  std::vector<TM_OTH::point_in_simulation> sim_points;

  auto most_recent = [] (std::vector<TM_OTH::point_in_simulation> &v, bool up, bool dn, bool le, bool ri) {
    int i = (int)v.size() - 1;
    while (i >= 0 && (v[i].is_pressed["up"] != up || v[i].is_pressed["dn"] != dn || v[i].is_pressed["le"] != le || v[i].is_pressed["ri"] != ri))
      i--;
    return i;
  };

  std::set<std::pair<double, std::vector<TM_OTH::point_in_simulation>>, compare_bfs> ss;
  ss.insert(make_pair(0.0, sim_points));

  double fit;
  bool did_end;
  int times_cut_from_set = 0, mr;
  while (!ss.empty()) {
    auto this_sim = *ss.begin();
    ss.erase(ss.begin());

    std::cout << "set has " << (int)ss.size() << " simulations remaining\n";
    ///wish I had this line LOL

    int write_at_time = (int)this_sim.second.size() * tmc.bfs_window_time_ms; ///when should I make the next move
    int cutoff_time = write_at_time + tmc.bfs_add_time_ms; ///till when should I run this

    this_sim.second.push_back(TM_OTH::point_in_simulation());
    this_sim.second.back().run_after_this_time = write_at_time;

    this_sim.second.back().is_pressed["up"] = true;
    std::tie(fit, did_end) = run_simulation(tmc, proc, this_sim.second, cutoff_time);
    if (fit > 0) {
      ss.insert(make_pair(fit, this_sim.second));
    }
    this_sim.second.back().is_pressed["up"] = false;


    mr = most_recent(this_sim.second, 1, 0, 1, 0); /// up & le
    if (mr == -1 || ((int)this_sim.second.size() - mr) * tmc.bfs_window_time_ms >= tmc.time_between_diff_steer) {
      this_sim.second.back().is_pressed["up"] = true;
      this_sim.second.back().is_pressed["ri"] = true;
      std::tie(fit, did_end) = run_simulation(tmc, proc, this_sim.second, cutoff_time);
      if (fit > 0) {
        ss.insert(make_pair(fit, this_sim.second));
      }
      this_sim.second.back().is_pressed["up"] = false;
      this_sim.second.back().is_pressed["ri"] = false;
    }


    mr = most_recent(this_sim.second, 1, 0, 0, 1); /// up & ri
    if (mr == -1 || ((int)this_sim.second.size() - mr) * tmc.bfs_window_time_ms >= tmc.time_between_diff_steer) {
      this_sim.second.back().is_pressed["up"] = true;
      this_sim.second.back().is_pressed["le"] = true;
      std::tie(fit, did_end) = run_simulation(tmc, proc, this_sim.second, cutoff_time);
      if (fit > 0) {
        ss.insert(make_pair(fit, this_sim.second));
      }
      this_sim.second.back().is_pressed["up"] = false;
      this_sim.second.back().is_pressed["le"] = false;
    }

    if ((int)ss.size() > tmc.ss_size_cutoff_point) {
      times_cut_from_set++;
      std::cout << "---CUTTING FROM SET: " << times_cut_from_set << " cut---\n";

      std::vector<std::pair<double, std::vector<TM_OTH::point_in_simulation>>> cut;

      for (auto x: ss)
        cut.push_back(x);

      ss.clear();

      std::sort(cut.begin(), cut.end(), [](
                const std::pair<double, std::vector<TM_OTH::point_in_simulation>> a,
                const std::pair<double, std::vector<TM_OTH::point_in_simulation>> b) {
                return a.first > b.first;
                });

      cut.resize(tmc.ss_size_cutoff_cut_to);

      for (auto x: cut)
        ss.insert(x);
    }
  }
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
