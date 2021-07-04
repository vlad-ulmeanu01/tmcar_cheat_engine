#ifndef __TMC_UTILITY_H
#define __TMC_UTILITY_H

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
#include <sstream>
#include <set>
#include <cmath>
#include <algorithm>

#define DEBUG 0

namespace TM_OTH
{
  struct point_of_interest {
    point_of_interest () {
      pos = std::make_tuple(0, 0, 0);
      best_before_time = 0;
      score_upon_completion = 0;
      meet_radius = 0;
      has_reached = false;
    }
    std::tuple<float, float, float> pos;
    int best_before_time;  /// measured in milliseconds (UINT ingame)
    int score_upon_completion;
    float meet_radius;
    bool has_reached;
  };

  struct point_in_simulation {
    point_in_simulation () {
      run_after_this_time = 0;
      is_pressed["up"] = is_pressed["dn"] = is_pressed["le"] = is_pressed["ri"] = false;
    }
    int run_after_this_time;
    std::unordered_map<std::string, bool> is_pressed; /// accepts "up", "dn", "le", "ri"
  };

  float square (float a);

  template<typename T>
  bool rational_cmp (T a, T b);

  float squared_distance (std::tuple<float, float, float> a, std::tuple<float, float, float> b);

  template<typename T>
  std::tuple<float, float, float> make_tuple_from_container (T cont, std::vector<std::string> s);

  void store_simulation (std::vector<point_in_simulation> &v);

  void get_simulation (std::string filename, std::vector<point_in_simulation> &v);
}

#endif
