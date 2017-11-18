/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *	License: MIT (More info in LICENSE.md)
*/

#include "email.h"

/*
 * Email je třída pro vytváření objektů, které uchovávají informace
 * o klientových emailech. Každý email tak obsahuje důležité informace
 * pro jeho zpracování (např. zda je určen ke smazání).
*/

/*
 * Konstruktor pro vytvoření objektu Email a přiřazení unikátního ID.
*/

Email::Email(int id) { this->id = id; }

/*
 * Getter pro získání ID emailu.
*/

int Email::getID() { return this->id; }
