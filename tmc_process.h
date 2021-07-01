#ifndef __TMC_PROCESS_H
#define __TMC_PROCESS_H

#include "tmc_utility.h"

class PROCESS_T {
private:
  HWND hwnd;
  LPCSTR name;
  DWORD processId;
  HANDLE hProcess;

public:
  void get_process_by_name (LPCSTR name);

  template<typename T>
  T read_value_from_address (DWORD address);

  template<typename T>
  BOOL write_value_to_address (DWORD address, T value);
};

#endif
