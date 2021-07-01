#include "tmc_utility.cpp"

template std::tuple<float, float, float> make_tuple_from_container<std::unordered_map<std::string, float> >
(std::unordered_map<std::string, float> cont, std::vector<std::string> s);
