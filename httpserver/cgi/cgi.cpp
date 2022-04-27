#include <iostream>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string>
using namespace std;

void handle(string parameter) {
  string part1, part2;
  int x = 0, y = 0;
  size_t pos = parameter.find('&');
  if(pos != parameter.size()) {
    part1 = parameter.substr(0, pos);
    part2 = parameter.substr(pos + 1);
  }
  size_t pos1 = part1.find('=');
  if(pos1 != part1.size()) {
    x = atoi(part1.substr(pos1 + 1).c_str());
  }
  size_t pos2 = part2.find('=');
  if(pos2 != part2.size()) {
    y = atoi(part2.substr(pos2 + 1).c_str());
  }
  cout << "<head>" << x << " + " << y << " = " << x + y << endl;
  cout << "<h1>" << x << " - " << y << " = " << x - y << "</h1>" << endl;
  cout << "<h2>" << x << " * " << y << " = " << x * y << "</h2>" << endl;
  cout << "<h3>" << x << " / " << y << " = " << x / y << "</h3>"  << "</head>" << endl;

}

int main() {
  //cout << "hello cgi" << endl;
  string method = getenv("METHOD_ENV");
  //cout << getenv("PARAMETER_ENV") << endl;
  //cout << method << endl;
  string parameter;
  if(strcasecmp(method.c_str(), "get") == 0) {
    //cout << method << endl;
    if(!getenv("PARAMETER_ENV")) {
      cout << "PARAMETER_ENV ERROR" << endl;
    }
    else {
      parameter = getenv("PARAMETER_ENV"); 
      cout << parameter << endl;
    }
  }
  else if(strcasecmp(method.c_str(), "post") == 0) {
    //参数在管道里面，进程替换前把文件描述符0重定向到了pipe_in[0]指向的管道
    int size = atoi(getenv("CONTENT-LENGTH"));
    while(size--) {
      char c;
      read(0, &c, 1);
      parameter += c;
    }
  }
  else {

  }

  handle(parameter);

  return 0;
}
