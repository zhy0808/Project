#pragma once

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <strings.h>
#include <unordered_map>
#include "Log.hpp"

#define BACKLOG 5

class Sock {
public:
  static int Socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
      LOG("Fatal", "socket error");
      exit(SocketErr);
    }
    return sock;
  } 
  static void Bind(int lsock, int port) {
    struct sockaddr_in host;
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(lsock, (struct sockaddr*)&host, sizeof(host)) < 0) {
      LOG("Fatal", "bind error");
      exit(BindErr);
    }

  } 

  static void SetSockOpt(int lsock) {
    int opt = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  }
  static void Listen(int lsock) {
    if(listen(lsock, BACKLOG) < 0) {
      LOG("Fatal", "listen error");
      exit(ListenErr);
    }
  }
  static int Accept(int lsock) {
    struct sockaddr_in peer;
    socklen_t len = sizeof(peer);
    int sock = accept(lsock, (struct sockaddr*)&peer, &len);
    if(sock < 0) {
      LOG("Warring", "accpet error");

    }
    return sock;
  }
};
