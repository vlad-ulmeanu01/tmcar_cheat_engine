#include "tmc_utility.h"

namespace TM_OTH
{
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
}

