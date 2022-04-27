#pragma once 
#include <iostream>
#include <string>
#include <sys/time.h>
using namespace std;


#define Notice  1
#define Warring 2
#define Error   3
#define Fatal   4

enum ERR {
  SocketErr = 1,
  BindErr,
  ListenErr,
  AcceptErr 
};
#define LOG(level, message) Log(#level, message, __FILE__, __LINE__)
void Log(string level, string message, string filename, size_t line) {
  struct timeval t;
  gettimeofday(&t, nullptr);
  cout << "[" << level << "]" << "[" << message << "]" << "[" << filename << "]" << "[" << line << "]" << "[" << t.tv_sec << "]" << endl;
}







