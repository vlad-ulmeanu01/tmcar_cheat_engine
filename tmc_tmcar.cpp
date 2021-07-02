#include "tmc_utility.h"
#include "tmc_process.h"
#include "tmc_tmcar.h"

DWORD TM_CAR::interpret_string_into_hex (std::string s) {
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

void TM_CAR::init_keymapping () {
  key_mapping.clear();
  key_mapping["up"] = 'i';
  key_mapping["dn"] = 'k';
  key_mapping["le"] = 'j';
  key_mapping["ri"] = 'l';
  key_mapping["reset"] = 'q';
}

template void TM_CAR::parse_input_file_to_umap(std::string filename, std::unordered_map<std::string, DWORD> &write_to);
template void TM_CAR::parse_input_file_to_umap(std::string filename, std::unordered_map<std::string, float> &write_to);

template<typename T>
void TM_CAR::parse_input_file_to_umap(std::string filename, std::unordered_map<std::string, T> &write_to) {
  write_to.clear();

  std::ifstream fin (filename);
  assert(fin.is_open());

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

void TM_CAR::parse_points_of_interest(std::string filename) {
  points_of_interest.clear();

  std::ifstream fin (filename);
  std::string line;

  ///splits the given string in: first<space>second
  std::function<std::pair<std::string, std::string>(std::string)> get_until_space = [] (std::string line) {
    int pos_space = line.find(" ");
    assert(pos_space != -1);

    return std::make_pair(line.substr(0, pos_space), line.substr(pos_space + 1, (int)line.size() - 1 - pos_space));
  };

  std::string str;
  while (std::getline(fin, line)) {
    int pos_space = line.find(" ");
    if (pos_space == -1) ///consider the line with no spaces a comment (ie the first one: "pos1,pos2,vel")
      continue;

    points_of_interest.push_back(TM_OTH::point_of_interest());

    std::tie(str, line) = get_until_space(line);
    std::get<0>(points_of_interest.back().pos) = atof(str.c_str());

    std::tie(str, line) = get_until_space(line);
    std::get<1>(points_of_interest.back().pos) = atof(str.c_str());

    std::tie(str, line) = get_until_space(line);
    std::get<2>(points_of_interest.back().pos) = atof(str.c_str());

    std::tie(str, line) = get_until_space(line);
    points_of_interest.back().best_before_time = atoi(str.c_str());

    std::tie(str, line) = get_until_space(line);
    points_of_interest.back().score_upon_completion = atoi(str.c_str());
    points_of_interest.back().meet_radius = atof(line.c_str());
  }

  if (DEBUG) {
    std::cout << "points of interest for " << filename << '\n';
    for (auto x: points_of_interest) {
      std::cout << std::get<0>(x.pos) << ' ' << std::get<1>(x.pos) << ' ' << std::get<2>(x.pos) << ' ' << x.best_before_time << ' ' << x.score_upon_completion << ' ' << x.meet_radius << '\n';
    }
    std::cout << std::flush;
  }

  fin.close();
}

void TM_CAR::parse_other_data (std::string filename) {
  std::ifstream fin (filename);
  assert(fin.is_open());

  std::string line;

  for (int cnt = 0; cnt < 5; cnt++) {
    std::getline(fin, line);
    int pos_space = line.find(" ");
    assert(pos_space != -1);
    std::string before_space = line.substr(0, pos_space),
                after_space = line.substr(pos_space + 1, (int)line.size() - 1 - pos_space);

    if (cnt == 0) {
      assert(before_space == "total_no_checkpoints");
      total_no_checkpoints = atoi(after_space.c_str());
    } else if (cnt == 1) {
      assert(before_space == "ss_size_cutoff_point");
      ss_size_cutoff_point = atoi(after_space.c_str());
    } else if (cnt == 2) {
      assert(before_space == "ss_size_cutoff_cut_to");
      ss_size_cutoff_cut_to = atoi(after_space.c_str());
    } else if (cnt == 3) {
      assert(before_space == "bfs_window_time_ms");
      bfs_window_time_ms = atoi(after_space.c_str());
    } else if (cnt == 4) {
      assert(before_space == "bfs_add_time_ms");
      bfs_add_time_ms = atoi(after_space.c_str());
    }
  }

  if (DEBUG) {
    std::cout << "total_no_checkpoints = " << total_no_checkpoints << '\n';
    std::cout << std::flush;
  }

  fin.close();
}

void TM_CAR::update_from_memory (PROCESS_T &proc) {
  values["pos_x"] = proc.read_value_from_address<float>(value_addresses["pos_x"]);
  values["pos_y"] = proc.read_value_from_address<float>(value_addresses["pos_y"]);
  values["pos_z"] = proc.read_value_from_address<float>(value_addresses["pos_z"]);
  values["timer"] = proc.read_value_from_address<int>(value_addresses["timer"]);
  values["ckpts"] = proc.read_value_from_address<int>(value_addresses["ckpts"]);
  values["speed"] = proc.read_value_from_address<int>(value_addresses["speed"]);
  values["distl"] = proc.read_value_from_address<int>(value_addresses["distl"]);
}

/// returns true if at least one POI has been passed through since last time.
bool TM_CAR::update_points_of_interest () {
  std::tuple<float, float, float> now_pos = TM_OTH::make_tuple_from_container<std::unordered_map<std::string, float> >
                                            (values, {"pos_x", "pos_y", "pos_z"});

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

double TM_CAR::fitness_function () {
  double score = 0;
  for (TM_OTH::point_of_interest &poi: points_of_interest)
    if (poi.has_reached)
      score += poi.score_upon_completion;

  score += 100 * values["ckpts"];
  score += values["speed"] / 4;
  score += values["distl"] / 5;
//  if (values["timer"] > 0)
//    score += 250000 / (values["timer"] + 1); /// incentivizes earlier runs

  if (TM_OTH::rational_cmp<float>(values["ckpts"], total_no_checkpoints))
    score *= 2;

  return score;
}

void TM_CAR::restart_race (PROCESS_T &proc) {
  for (TM_OTH::point_of_interest &poi: points_of_interest)
    poi.has_reached = false;

  assert(values["ckpts"] <= total_no_checkpoints);

  if (!TM_OTH::rational_cmp<float>(values["ckpts"], total_no_checkpoints)) {
    tap_key(key_mapping["reset"]);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } else {
    while (!TM_OTH::rational_cmp<float>(values["ckpts"], 0)) {
      tap_key('\n');
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      update_from_memory(proc);
    }
  }
}

void TM_CAR::tap_key (char ch) {
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.dwFlags = KEYEVENTF_SCANCODE;
  input.ki.wScan = MapVirtualKey(LOBYTE(VkKeyScan(ch)), 0);

  SendInput(1, &input, sizeof(input));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  input.ki.dwFlags = (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP);
  SendInput(1, &input, sizeof(input));
}

/// will tap simultaneously any combination of the up/dn/le/ri keys.
void TM_CAR::tap_keys (TM_OTH::point_in_simulation point) {
  /// key_mapping is an unordered_map: key_mapping["up"] = 'i' etc
  /// point.is_pressed is an unordered_map: is_pressed["up"] = T/F

  int no_of_keys_pressed = 0;
  for (std::pair<std::string, bool> x: point.is_pressed)
    if (x.second)
      no_of_keys_pressed++;

  if (no_of_keys_pressed == 0)
    return;

  INPUT input[2 * no_of_keys_pressed] = {0};

  int cnt = 0;
  for (std::pair<std::string, bool> x: point.is_pressed)
    if (x.second) {
      input[cnt].type = INPUT_KEYBOARD;
      input[cnt].ki.dwFlags = KEYEVENTF_SCANCODE;
      input[cnt].ki.wScan = MapVirtualKey(LOBYTE(VkKeyScan(key_mapping[x.first])), 0);
      cnt++;

      input[cnt] = input[cnt - 1];
      cnt++;
    }

  SendInput(2 * no_of_keys_pressed, input, sizeof(INPUT));
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  for (cnt = 1; cnt < 2 * no_of_keys_pressed; cnt += 2)
    input[cnt].ki.dwFlags |= KEYEVENTF_KEYUP;

  SendInput(2 * no_of_keys_pressed, input, sizeof(INPUT));
}
