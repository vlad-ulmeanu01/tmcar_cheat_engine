#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <typeinfo>
#include <unordered_map>
#include <chrono>
#include <thread>

#define DEBUG 0

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
};

void move_car_by_ox (TM_CAR &tmc, PROCESS_T &proc) {
  tmc.values["pos_x"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]);

  for (int i = 0; i < 30; i++) {
    tmc.values["pos_x"]++;
    proc.write_value_to_address<float>(tmc.value_addresses["pos_x"], tmc.values["pos_x"]);
  }
}

void get_pos_indefinitely(TM_CAR &tmc, PROCESS_T &proc) {
  std::chrono::milliseconds timespan(1000);

  while (1) {
    tmc.values["pos_x"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_x"]);
    tmc.values["pos_y"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_y"]);
    tmc.values["pos_z"] = proc.read_value_from_address<float>(tmc.value_addresses["pos_z"]);

    std::cout << tmc.values["pos_x"] << ' ' << tmc.values["pos_y"] << ' ' << tmc.values["pos_z"] << '\n';

    std::this_thread::sleep_for(timespan);
  }
}

int main() {
  TM_CAR tmc;
  ///tmc.parse_input_file<float>("tm_values.txt", tmc.values);
  tmc.parse_input_file<DWORD>("tm_value_addresses.txt", tmc.value_addresses);

  PROCESS_T proc;
  proc.get_process_by_name("TrackMania United Forever");

  //move_car_by_ox(tmc, proc);
  get_pos_indefinitely(tmc, proc);

  return 0;
}
