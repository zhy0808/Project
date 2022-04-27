#pragma once 
#include "Protocol.hpp"
#include "Sock.hpp"
#include "PthreadPool.hpp"

using namespace std;


class HttpServer {
private:
  int port;
  int lsock;
  PthreadPool* pt;
public:
  HttpServer(int _port = 8080)
    :port(_port)
     , lsock(-1){
      pt = new PthreadPool(5);
     }
  void init() {
    //服务器正在向客户端发送数据时，客户端关闭连接，服务器再往套接字中写数据时，操作系统会向服务器进程发送SIGPIPE信号，服务器就会被终止，这里要忽略该信号，防止被终止
    signal(SIGPIPE, SIG_IGN);
    lsock = Sock::Socket();
    Sock::SetSockOpt(lsock);
    Sock::Bind(lsock, port);
    Sock::Listen(lsock);
    //初始化线程池
    pt->init();
  }
  void start() {
    while(1) {
      int sock = Sock::Accept(lsock);
      if(sock < 0) {
        LOG(Warring, "accept error");
        continue;
      }
      LOG(Notice, "get a new link"); 
      
      
      Task* t = new Task(sock);
      pt->PutTask(t);
      
      
      //pthread_t tid;
      //在堆上开辟空间存储获取到的套接字，防止正在创建线程时，主线程又获取到了新的套接字
      //int* s = new int(sock);
      //pthread_create(&tid, nullptr, Entry::Service, s);
      //pthread_detach(tid);
      
    }
  }

  ~HttpServer() {
    close(lsock);
  }
};
