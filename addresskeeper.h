#ifndef addresskeeper_h
#define addresskeeper_h

#include "gadget.h"

class AddressKeeper {
public:
  double* addr;
  char* name;
  AddressKeeper() { addr = 0; name = 0; };
  ~AddressKeeper();
  AddressKeeper(const AddressKeeper& a)
    : addr(a.addr), name(new char[strlen(a.name) + 1])
    { strcpy(name, a.name); };
  void operator = (double* address) { addr = address; };
  void operator = (double& value) { addr = &value; };
  int operator == (const double& val) { return (addr == &val); };
  int operator == (const double* add) { return (addr == add); };
  void operator = (const char* str1) {
    if (name != 0)
      delete[] name;
    name = new char[strlen(str1) + 1];
    strcpy(name, str1);
    delete[] str1;
  };
  void operator = (const AddressKeeper a) {
    if (name != 0)
      delete[] name;
    name = new char[strlen(a.name) + 1];
    strcpy(name, a.name);
    addr = a.addr;
  };
};

#endif