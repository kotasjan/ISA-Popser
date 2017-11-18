/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *	License: MIT (More info in LICENSE.md)
*/

/*
 * Hlavičkový soubor common.h sdružuje konstanty a knihovny společné pro všechny
 * ostatní části projektu.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <algorithm>

using namespace std;

#define MAX_NAME_LENGHT 20

#define MAX_CLIENTS 20

#define BUF_SIZE 1024