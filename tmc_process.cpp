#include "tmc_process.h"

void PROCESS_T::get_process_by_name (LPCSTR name) {
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

template float PROCESS_T::read_value_from_address(DWORD address);
template int PROCESS_T::read_value_from_address(DWORD address);

template<typename T>
T PROCESS_T::read_value_from_address (DWORD address) {
  T value = 0;
  ReadProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), NULL);

  if (DEBUG) {
    std::cout << "Have read value of type " << typeid(T).name() << " with value: " << value << '\n';
    std::cout << std::flush;
  }

  return value;
}

template BOOL PROCESS_T::write_value_to_address(DWORD address, float value);
template BOOL PROCESS_T::write_value_to_address(DWORD address, int value);

template<typename T>
BOOL PROCESS_T::write_value_to_address (DWORD address, T value) {
  SIZE_T bytes_written = 0;
  BOOL result = WriteProcessMemory(hProcess, (LPVOID)address, &value, sizeof(T), &bytes_written);

  if (DEBUG && !result) {
    std::cout << "Couldn't write " << value << " to " << address << '\n';
    std::cout << std::flush;
  }

  return result;
}
