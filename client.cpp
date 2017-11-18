/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *	License: MIT (More info in LICENSE.md)
*/

#include "client.h"

/*
 * Client je třída pro vytváření objektů, které uchovávají informace
 * o připojených klientech. Po připojení klienta na server se vytvoří
 * objekt "Client" a klientovi se přidělí unikátní ID.
*/

/*
 * Konstruktor, který vytvoří klienta s konkrétním ID.
*/

Client::Client(int id) { this->id = id; }

/*
 * Setter pro nastavení booleovské proměnné transaction, která určuje,
 * zda se klient nachází v transakční fázi.
*/

void Client::SetTransactionPhase(bool b) { this->transaction = b; }

/*
 * Funkce SetHash() vytváří unikátní md5 hash složený z časového razítka,
 * hostname serveru a správného přihlašovacího hesla.
*/

void Client::SetHash(string myString, string right_password) {
  this->hash = md5(myString + right_password);
}

/*
 * Tento getter slouží k přístupu k proměnné transakction. Díky tomu můžeme
 * zjistit, zda se klient nachází v transakční fázi.
*/

bool Client::isInTransactionPhase() {
  if (this->transaction)
    return true;
  else
    return false;
}

/*
 * Getter getHash() vrací hash vytvořený funkcí SetHash().
*/

string Client::getHash() { return this->hash; }

/*
 * Getter vracící ID klienta.
*/

int Client::getID() { return this->id; }