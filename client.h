/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *  License: MIT (More info in LICENSE.md)
*/

#ifndef _client_h_
#define _client_h_

#include "common.h"
#include "md5.h"
#include "email.h"

using namespace std;

/*
 * Hlavičkový soubor pro třídu Client. Třída Client obsahuje základní informace
 * o připojeném klientovi.
*/

class Client {
 private:
  bool transaction = false;
  string hash;
  int id;

 public:
  vector<Email> emails;
  int socket, curEmails = 0, overalSize = 0;
  string username = "";

 public:
  Client(int id);
  void SetTransactionPhase(bool b);
  void SetHash(string myString, string right_password);
  bool isInTransactionPhase();

  int getID();
  string getHash();
};

#endif