/*
 *  Subject: ISA 2017/2018
 *  Program: Popser
 *  Author: Jan Kotas, xkotas07
 *  License: MIT (More info in LICENSE.md)
*/

#include "thread.h"

/*
 * Thread je třída reprezentující vlákno. Z důvodu zjednodušení
 * práce s vlákny a práce s mutexy byla vytvořena tato třída.
 * V programu jsou využity celkem dva mutexy a to obecný mutex
 * "mutex" a mutex pro výlučný přístup do transakční fáze "transaction".
*/

pthread_mutex_t Thread::mutex;
pthread_mutex_t Thread::transaction;

/*
 * Konstruktor třídy Thread.
*/

Thread::Thread() {}

/*
 * Funkce Create() slouží k vytvoření vlákna.
*/

int Thread::Create(void* Callback, void* args) {
  int t = 0;

  t = pthread_create(&this->tid, NULL, (void* (*)(void*))Callback, args);

  if (t) {
    cerr << "Error while creating threads." << endl;
    return t;
  } else {
    return 0;
  }
}

/*
 * Funkce Join() odstraní již nepotřebné vlákno.
*/

int Thread::Join() {
  pthread_join(this->tid, NULL);
  return 0;
}

/*
 * Funkce, která inicializuje obecný mutex.
*/

int Thread::InitMutex() {
  if (pthread_mutex_init(&Thread::mutex, NULL) < 0) {
    cerr << "Error while initializing mutex" << endl;
    return -1;
  } else {
    return 0;
  }
}

/*
 * Funkce LockMutex() žádá o přístup k mutexu a vlákno, které funkci
 * volá, musí čekat do doby, než je mutex odemčen jiným vláknem.
*/

int Thread::LockMutex() {
  if (pthread_mutex_lock(&Thread::mutex) == 0) {
    return 0;
  } else {
    return -1;
  }
}

/*
 * Funkce UnlockMutex() odemyká zamčený obecný mutex.
*/

int Thread::UnlockMutex() {
  if (pthread_mutex_unlock(&Thread::mutex) == 0) {
    return 0;
  } else {
    cerr << "Error while trying to release the lock" << endl;
    return -1;
  }
}

/*
 * Funkce, která inicializuje transakční mutex.
*/

int Thread::InitTransaction() {
  if (pthread_mutex_init(&Thread::transaction, NULL) < 0) {
    cerr << "Error while initializing transaction phase" << endl;
    return -1;
  } else {
    return 0;
  }
}

/*
 * Funkce LockTransaction() žádá o přístup do transakční fáze.
 * Vlákno čeká 3 sekundy na přístup k zámku a pokud jej nedostane,
 * funkce je ukončena neúspěchem a musí být volána opětovně.
*/

int Thread::LockTransaction() {
  struct timespec* wait;
  wait = (struct timespec*)(malloc(sizeof(struct timespec)));
  wait->tv_sec = 3;
  wait->tv_nsec = 0;

  if (pthread_mutex_timedlock(&Thread::transaction, wait) != 0) {
    return -1;
  } else {
    return 0;
  }
}

/*
 * Funkce UnlockTransaction() odemyká transakční fázi.
*/

int Thread::UnlockTransaction() {
  if (pthread_mutex_unlock(&Thread::transaction) == 0) {
    return 0;
  } else {
    cerr << "Error while trying to release the lock of transaction phase"
         << endl;
    return -1;
  }
}
