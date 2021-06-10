#define _WIN32_WINNT 0x0500
#include <windows.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <typeinfo>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <tuple>
#include <vector>

#define DEBUG 0

namespace TM_OTH {
  struct point_of_interest {
    std::tuple<float, float, float> pos;
    int best_before_time;  /// measured in milliseconds (UINT ingame)
    int score_upon_completion;
    float meet_radius;
    bool has_reached;
  };

  struct point_in_simulation {
    point_in_simulation () {
      run_after_this_time = 0;
      is_up_pressed = is_dn_pressed = is_le_pressed = is_ri_pressed = false;
    }
    int run_after_this_time;
    bool is_up_pressed, is_dn_pressed, is_le_pressed, is_ri_pressed;
  };

  float square (float a) {
    return a * a;
  }

  float squared_distance (std::tuple<float, float, float> a, std::tuple<float, float, float> b) {
    float ans = 0;

    ans += square(std::get<0>(a) - std::get<0>(b));
    ans += square(std::get<1>(a) - std::get<1>(b));
    ans += square(std::get<2>(a) - std::get<2>(b));

    return ans;
  }

  std::tuple<float, float, float> make_tuple_from_container (auto cont, std::vector<std::string> s) {
    assert((int)s.size() == 3);

    std::tuple<float, float, float> ans;

    std::get<0>(ans) = cont[s[0]];
    std::get<1>(ans) = cont[s[1]];
    std::get<2>(ans) = cont[s[2]];

    return ans;
  }
}

class PROCESS_T {
private:
  HWND hwnd;
  LPCSTR name;
  DWORD processId;
  HANDLE hProcess;

public:
  void get_process_by_name (LPCSTR name) {
    this->name = name;

    hwnd = FindWindow(NULL, name);
    assert(hwnd != NULL);

    GetWindowThreadProcessId(hwnd, &processId);

    if (DEBUG) {
      std::cout << "process Name: " << name << ", processId: " << processId << '\n';
      std::cout << std::flush;
    }

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
  }

  template<typename T>
  T read_value_from_address (DWORD address) {
    T value = 0;
    ReadProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);

    if (DEBUG) {
      std::cout << "Have read value of type " << typeid(T).name() << " with value: " << value << '\n';
      std::cout << std::flush;
    }

    return value;
  }

  template<typename T>
  BOOL write_value_to_address (DWORD address, T value) {
    SIZE_T bytes_written = 0;
    BOOL result = WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), &bytes_written);

    if (DEBUG && !result) {
      std::cout << "Couldn't write " << value << " to " << address << '\n';
      std::cout << std::flush;
    }

    return result;
  }
};

class TM_CAR {
private:
  DWORD interpret_string_into_hex (std::string s) {
    DWORD ans = 0;
    unsigned int p16 = 1;
    while ((int)s.size() > 0 && s.back() != 'x') {
      assert(s.back() != ' ');

      if (s.back() >= '0' && s.back() <= '9')
        ans += (s.back() - '0') * p16;
      else {
        assert(s.back() >= 'A' && s.back() <= 'F');
        ans += (s.back() - 'A' + 10) * p16;
      }
      p16 <<= 4;
      s.pop_back();
    }
    return ans;
  }
public:
  /**
    the dictionaries accept:
    vel_x, vel_y, vel_z;
    pos_x, pos_y, pos_z
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

  template<typename T>
  void parse_input_file(std::string filename, std::unordered_map<std::string, T> &write_to) {
    write_to.clear();

    std::ifstream fin (filename);

    std::string line;
    while (std::getline(fin, line)) {
      int pos_space = line.find(" ");

      assert(pos_space != -1);  /// if the input is correct I must have at least one space

      std::string before_space = line.substr(0, pos_space),
                  after_space = line.substr(pos_space + 1, (int)line.size() - 1 - pos_space);

      if (typeid(T).name() == typeid(float).name()) {
        /// I want to write in "values"
        float value = atof(after_space.c_str());
        values[before_space] = value;
      } else {
        /// I want to write in "value_addresses"
        assert(typeid(T).name() == typeid(DWORD).name());
        value_addresses[before_space] = interpret_string_into_hex(after_space);
      }
    }

    fin.close();
  }

  void update_from_memory (PROCESS_T &proc) {
    values["pos_x"] = proc.read_value_from_address<float>(value_addresses["pos_x"]);
    values["pos_y"] = proc.read_value_from_address<float>(value_addresses["pos_y"]);
    values["pos_z"] = proc.read_value_from_address<float>(value_addresses["pos_z"]);
    values["timer"] = proc.read_value_from_address<int>(value_addresses["timer"]);
  }

  /// returns true if at least one POI has been passed through since last time.
  bool update_points_of_interest () {
    std::tuple<float, float, float> now_pos = TM_OTH::make_tuple_from_container(values, {"pos_x", "pos_y", "pos_z"});

    bool did_change = false;
    for (TM_OTH::point_of_interest &poi: points_of_interest) {
      if (poi.has_reached == false &&
          TM_OTH::squared_distance(poi.pos, now_pos) < poi.meet_radius &&
          values["timer"] < poi.best_before_time) {
        poi.has_reached = true;
        did_change = true;
      }
    }

    if (DEBUG && did_change) {
      for (TM_OTH::point_of_interest poi: points_of_interest)
        std::cout << poi.has_reached << ' ';
      std::cout << '\n' << std::flush;
    }

    return did_change;
  }

  double fitness_function () {
    double score = 0;
    for (TM_OTH::point_of_interest &poi: points_of_interest)
      if (poi.has_reached)
        score += poi.score_upon_completion;

    return score;
  }

  void restart_race () {
    for (TM_OTH::point_of_interest &poi: points_of_interest)
      poi.has_reached = false;

    tap_key('q');
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  void tap_key (char ch) {
    SHORT key = VkKeyScan(ch);
    UINT mapped_key = MapVirtualKey(LOBYTE(key), 0);

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.wScan = mapped_key;

    SendInput(1, &input, sizeof(input));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    input.ki.dwFlags = (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP);
    SendInput(1, &input, sizeof(input));
  }

  void tap_keys (TM_OTH::point_in_simulation point) {
    if (point.is_up_pressed)
      tap_key('i');
    if (point.is_dn_pressed)
      tap_key('k');
    if (point.is_le_pressed)
      tap_key('j');
    if (point.is_ri_pressed)
      tap_key('l');
  }
};

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

    std::cout << tmc.values["pos_x"] << ' ' << tmc.values["pos_y"] << ' ' << tmc.values["pos_z"] << '\n';

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

    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

  sim_points.back().is_up_pressed = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_up_pressed = false;

  sim_points.back().is_up_pressed = true;
  sim_points.back().is_ri_pressed = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_up_pressed = false;
  sim_points.back().is_ri_pressed = false;

  sim_points.back().is_up_pressed = true;
  sim_points.back().is_le_pressed = true;
  depth_first_search(tmc, proc, sim_points);
  sim_points.back().is_up_pressed = false;
  sim_points.back().is_le_pressed = false;

  /// TODO pune mai multe bobite pe circuit

//  sim_points.back().is_ri_pressed = true;
//  depth_first_search(tmc, proc, sim_points);
//  sim_points.back().is_ri_pressed = false;

  //depth_first_search(tmc, proc, sim_points);
  /// ^ nu are sens sa nu faci nimic daca tot dai tap

  sim_points.pop_back();
}

int main() {
  TM_CAR tmc;
  ///tmc.parse_input_file<float>("tm_values.txt", tmc.values);
  tmc.parse_input_file<DWORD>("tm_value_addresses.txt", tmc.value_addresses);

  /// TODO fa functie care parseaza points_of_interest
  tmc.points_of_interest.push_back({std::make_tuple(321.650, 9.359, 507.773),  5000, 100, 2.0, false});
  tmc.points_of_interest.push_back({std::make_tuple(323.160, 9.359, 579.627),  7500, 100, 2.0, false});
  tmc.points_of_interest.push_back({std::make_tuple(251.292, 9.359, 607.919), 20000, 100, 5.0, false});

  PROCESS_T proc;
  proc.get_process_by_name("TrackMania United Forever");

  //teleport_car_by_ox(tmc, proc);
  //get_pos_indefinitely(tmc, proc);
  //move_car_by_ox(tmc, proc);
  //checkpoint_reach(tmc, proc);

  std::vector<TM_OTH::point_in_simulation> sim_points;

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  depth_first_search(tmc, proc, sim_points);

  return 0;
}
