#pragma once 
#include <iostream>
#include <queue>
#include <pthread.h>
#include "Protocol.hpp"
using namespace std;
//声明一个函数指针类型，返回值为void，参数为int
typedef void(*hander_p)(int);
class Task {
private:
  int sock;
  hander_p hander;
public:
  Task(int s)
    :sock(s)
     ,hander(Entry::Service)  //用要处理套接字的函数来初始化这个函数指针
  {

  }
  void Run() {
    hander(sock);
  }
};

class PthreadPool {
private:
  int cap;
  queue<Task*> q;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  void LockQueue() {
    pthread_mutex_lock(&lock);
  } 
  void UnlockQueue() {
    pthread_mutex_unlock(&lock);
  }
  void Wait() {
    pthread_cond_wait(&cond, &lock);
  }
  bool IsEmpty() {
    return q.empty();
  }
  Task* GetTask() {
    Task* t = q.front();
    q.pop();
    return t;
  }
  static void* Routine(void* args) {
    PthreadPool* p = (PthreadPool*)args;
    p->LockQueue();
    while(p->IsEmpty()) {
      p->Wait();
    }
    Task* t = p->GetTask();
    p->UnlockQueue();
    t->Run();
    delete t;
  }
  void WakeUpPthread() {
    pthread_cond_signal(&cond);
  }
public:
  PthreadPool(int _cap = 8)
    :cap(_cap) 
  {

  }

  void init() {
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&cond, nullptr);
    pthread_t pid;
    for(int i = 0; i < cap; ++i) {
      pthread_create(&pid, nullptr, Routine, this);
      pthread_detach(pid);
    }
  }
  void PutTask(Task* t) {
    LockQueue();
    q.push(t);
    UnlockQueue();
    WakeUpPthread();
  }

   
  ~PthreadPool() {
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
  }

};
