#ifndef _email_h_
#define _email_h_

#include "common.h"

using namespace std;

class Email {
 public:
  string name, uniqueName, content;
  bool deleteFlag = false;
  int size;

 private:
  int id;

 public:
  Email(int id);
  int getID();
  void print();

};

#endif