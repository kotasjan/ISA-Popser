#ifndef __XPOPSER_H
#define __XPOPSER_H

#include "common.h"
#include "thread.h"
#include "server.h"

using namespace std;

void load_right_user();
void check_path_to_files();
void reset();

void help();
void error(int code);
void printPar();
void signal_handler(int i);

void loadParams(int argc, char* argv[]);

struct params {
  string authFile;
  string directoryPath;
  uint16_t port = 0;
  int cFlag = 0;
  int resetFlag = 0;
  int runMode = 0;
  pid_t pid;
} par;

string right_username = "";
string right_password = "";

#endif