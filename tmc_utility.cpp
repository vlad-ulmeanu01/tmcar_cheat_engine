#include "tmc_utility.h"

namespace TM_OTH
{
  float square (float a) {
    return a * a;
  }

  template bool rational_cmp (float a, float b);
  template bool rational_cmp (double a, double b);

  template<typename T>
  bool rational_cmp (T a, T b) {
    if (fabs(a - b) < 0.00001)
      return true;
    return false;
  }

  float squared_distance (std::tuple<float, float, float> a, std::tuple<float, float, float> b) {
    float ans = 0;

    ans += square(std::get<0>(a) - std::get<0>(b));
    ans += square(std::get<1>(a) - std::get<1>(b));
    ans += square(std::get<2>(a) - std::get<2>(b));

    return ans;
  }

  template std::tuple<float, float, float> make_tuple_from_container
           (std::unordered_map<std::string, float>, std::vector<std::string> s);

  template<typename T>
  std::tuple<float, float, float> make_tuple_from_container (T cont, std::vector<std::string> s) {
    assert((int)s.size() == 3);

    std::tuple<float, float, float> ans;

    std::get<0>(ans) = cont[s[0]];
    std::get<1>(ans) = cont[s[1]];
    std::get<2>(ans) = cont[s[2]];

    return ans;
  }

  void store_simulation (std::vector<point_in_simulation> &sim_points) {
    std::ostringstream stringStream;
    stringStream << time(0);
    std::string filename = "run_" + stringStream.str() + ".txt";

    std::ofstream fout (filename);

    for (point_in_simulation x: sim_points) {
      fout << x.run_after_this_time << ' ';
      fout << x.is_pressed["up"];
      fout << x.is_pressed["dn"];
      fout << x.is_pressed["le"];
      fout << x.is_pressed["ri"] << '\n';
    }

    fout.close();
  }

  void get_simulation (std::string filename, std::vector<point_in_simulation> &v) {
    std::ifstream fin (filename);
    assert(fin.is_open());

    v.clear();
    std::string line;
    while (std::getline(fin, line)) {
      int pos_space = line.find(" ");
      assert(pos_space != -1); /// if the input is correct I must have at least one space

      std::string before_space = line.substr(0, pos_space),
                  after_space = line.substr(pos_space + 1, (int)line.size() - 1 - pos_space);

      v.push_back(point_in_simulation());
      v.back().run_after_this_time = atoi(before_space.c_str());

      v.back().is_pressed["up"] = after_space[0] - '0';
      v.back().is_pressed["dn"] = after_space[1] - '0';
      v.back().is_pressed["le"] = after_space[2] - '0';
      v.back().is_pressed["ri"] = after_space[3] - '0';
    }

    fin.close();
  }
}
