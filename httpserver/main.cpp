#include "Httpserver.hpp"
#include "Log.hpp"

void usage(char* arg) {
  cout << arg << " port " << endl;
  cout << arg << endl;
}

int main(int argc, char* argv[]) {
  if(argc != 2 && argc != 1) {
    usage(argv[0]);
    exit(1);
  }
  HttpServer* h;
  if(argc == 2) {
    h = new HttpServer(atoi(argv[1]));
  }
  else {
    h = new HttpServer();
  }
  h->init();
  h->start();
  return 0;
}
