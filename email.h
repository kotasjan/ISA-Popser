/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *	License: MIT (More info in LICENSE.md)
*/

#ifndef _email_h_
#define _email_h_

#include "common.h"

/*
 * Hlavičkový soubor třídy Email. Třída Email vytváří instance, které
 * sdružují informace o přijatých emailech.
*/

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