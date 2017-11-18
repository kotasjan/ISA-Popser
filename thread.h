#ifndef _thread_h_
#define _thread_h_

#include "common.h"

class Thread {
 public:
  pthread_t tid;

 private:
  static pthread_mutex_t mutex;
  static pthread_mutex_t transaction;

 public:
  Thread();
  int Create(void* Callback, void* args);
  int Join();

  static int InitMutex();
  static int LockMutex();
  static int UnlockMutex();

  static int InitTransaction();
  static int LockTransaction();
  static int UnlockTransaction();
};

#endif