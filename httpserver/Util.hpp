#pragma once 
#include "Sock.hpp"
#include "Log.hpp"
#include <sstream>
class Util {
public:
  static void GetLine(const int sock, string& line) {
    char c = ' ';
    while(c != '\n') {
      ssize_t s = recv(sock, &c, 1, 0);
      if(s > 0) {
        if(c == '\r') {
          if(recv(sock, &c, 1, MSG_PEEK) > 0) {
            if(c == '\n') 
              recv(sock, &c, 1, 0);
          } 
          c = '\n';
        }
        line += c;
      }
    }
  }
  
  static void StringPause(string line, string& method, string& uri, string& version) {
    stringstream s(line);
    s >> method >> uri >> version;
  
  }

  static void HeaderLineToKV(string& line, string& k, string& v) {
    size_t it = line.find(": ");
    if(it != line.size()) {
      k = line.substr(0, it);
      v = line.substr(it + 2, line.size());
      v[v.size() - 1] = 0;
    }

  }
};

