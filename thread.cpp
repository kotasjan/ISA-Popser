#include "thread.h"

// Need to actually "allocate" static member
pthread_mutex_t Thread::mutex;
pthread_mutex_t Thread::transaction;

Thread::Thread() {}

int Thread::Create(void* Callback, void* args) {
  int t = 0;

  // Supercreepy typecast
  t = pthread_create(&this -> tid, NULL, (void* (*)(void*))Callback, args);

  if (t) {
    cerr << "Error while creating threads." << endl;
    return t;
  } else {
    // cout << "Thread successfully created." << endl;
    return 0;
  }
}

int Thread::Join() {
  pthread_join(this -> tid, NULL);
  return 0;
}

int Thread::InitMutex() {
  if (pthread_mutex_init(&Thread::mutex, NULL) < 0) {
    cerr << "Error while initializing mutex" << endl;
    return -1;
  } else {
    // cout << "Mutex initialized." << endl;
    return 0;
  }
}

/*
    LockMutex():
    Blocks until mutex becomes available
*/
int Thread::LockMutex() {
  if (pthread_mutex_lock(&Thread::mutex) == 0) {
    // cout << "Someone acquired the lock!" << endl;
    return 0;
  } else {
    // cerr << "Error while trying to acquire the lock" << endl;
    return -1;
  }
}

int Thread::UnlockMutex() {
  if (pthread_mutex_unlock(&Thread::mutex) == 0) {
    // cout <<"Released the lock!" << endl;
    return 0;
  } else {
    cerr << "Error while trying to release the lock" << endl;
    return -1;
  }
}

int Thread::InitTransaction() {
  if (pthread_mutex_init(&Thread::transaction, NULL) < 0) {
    cerr << "Error while initializing transaction phase" << endl;
    return -1;
  } else {
    // cout << "Transaction initialized." << endl;
    return 0;
  }
}

/*
    LockTransaction():
    Blocks until transaction phase becomes available
*/
int Thread::LockTransaction() {

  struct timespec *wait;
  wait=(struct timespec *)(malloc(sizeof(struct timespec)));
  wait->tv_sec=3;
  wait->tv_nsec=0;

  if(pthread_mutex_timedlock(&Thread::transaction,wait) != 0){
   // cerr << "Error while trying to acquire the transaction lock" << endl;
   return -1;
  } else {
    // cout << "Someone acquired the transaction lock!" << endl;
    return 0;
  }
  
}

int Thread::UnlockTransaction() {
  if (pthread_mutex_unlock(&Thread::transaction) == 0) {
    // cout <<"Released the lock of transaction phase!" << endl;
    return 0;
  } else {
    cerr << "Error while trying to release the lock of transaction phase" << endl;
    return -1;
  }
}