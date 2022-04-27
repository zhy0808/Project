#pragma once

#include "Sock.hpp"
#include "Util.hpp"

#define WWWROOT "wwwroot"
#define HOMEPAGE "index.html"
#define VERSION "HTTP/1.0"

static string CodeToDesc(int code) {
  string desc;
  switch(code) {
    case 200:
      desc = "OK";
      break;
    case 404:
      desc = "Not Found";
      break;
    default:
      desc = "OK";
      break;
  }
  return desc;
}
string SuffixToDesc(const string& suffix) {
  if(suffix == ".html" || suffix == ".htm") {
    return "text/html";
  } 
  else if(suffix == ".js") {
    return "application/x-javascript";
  }
  else if(suffix == ".css") {
    return "text/css";
  }
  else if(suffix == ".jpg") {
    return "image/jpeg";
  }
  else if(suffix == ".png") {
    return "image/png";
  }
  else {
    return "text/html";
  }
}

class Request {
private:
  string request_line;            //请求行
  string method;                  //请求方法 
  string uri;                     //uri
  string path;                    //提取uri中要访问的路径
  struct stat st;                 //当前路径的文件属性
  string parameter;               //如果是get方法，提取uri中的参数部分
  string version;                 //版本号

  vector<string> request_header;  //请求头部
  unordered_map<string, string> kv;//将请求头部以key，value形式存储

  string blank;                   //空行
  string request_body;            //正文
  int content_length;              //如果有正文代表正文长度
  
  bool cgi;                       //表示当前是否要请求cgi
public:
  Request()
    :path(WWWROOT)
     ,blank("\n")
     ,content_length(-1)
     ,cgi(false) 
  {

  }
  bool IsNeedRecvBody() {
    if(strcasecmp(method.c_str(), "post") == 0 && content_length > 0) {
      return true;
    }
    return false;
  }
  void SetRequestLine(string line) {
    request_line = line;
  }
  void RequestLinePause() {
    Util::StringPause(request_line, method, uri, version);
    //LOG(Notice, method);
    //LOG(Notice, uri);
    //LOG(Notice, version);
  }
  void SetRequestHeader(string& line) {
    request_header.push_back(line);
  }
  void RequestHeaderPause() {
    for(string& s : request_header) {
      string k, v;
      Util::HeaderLineToKV(s, k, v);
      kv.insert({k, v});
      if(k == "Content_Length") {
        content_length = atoi(v.c_str());
      }
      //LOG(Notice, k);
      //LOG(Notice, v);
    }
  }
  int GetContentLength() {
    return content_length;
  }
  void SetRequestBody(string& body) {
    request_body = body;
  }
  bool IsMethodLegal() {
    if(strcasecmp(method.c_str(), "get") == 0 || strcasecmp(method.c_str(), "post") == 0) {
      return true;
    }
    return false;
  }

  void UriPause() {
    if(strcasecmp(method.c_str(), "get") == 0) {
      size_t pos = uri.find("?");
      if(pos != uri.size()) {
        path += uri.substr(0, pos);
        parameter = uri.substr(pos + 1);
      }
      else {
        path += uri;
      }
    }
    else {
      path += uri;
    }
    //LOG(Notice, path);
    //LOG(Notice, parameter); 
  }
  void IsAddHomePage() {
    //如果获取到的路径以/结尾，给他加上index.html
    if(path[path.size() - 1] == '/') {
      path += HOMEPAGE;
    }
      
    LOG(Notice, path);
  }
  //判断当前路径是否合法
  bool IsPathLegal() {
    if(stat(path.c_str(), &st) == 0) {
      return true;
    }
    return false;
  }
  bool IsDir() {
    if(S_ISDIR(st.st_mode)) {
      return true;
    }
    return false;
  }
  string GetPath() {
    return path;
  }
  void SetPath(string p) {
    path = p;
  }
  bool IsCgi() {
    if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH) {
      return true;
    }
    return false;
  }
  void SetCgi(bool b) {
    cgi = b;
  }
  bool GetCgi() {
    return cgi;
  }
  ssize_t GetFileSize() {
    return st.st_size;
  }
  string MakeSuffix() {
    size_t pos = path.find('.');
    string suffix;
    if(pos != path.size()) {
      suffix = path.substr(pos);
    }
    return suffix;
  }
  bool IsGet() {
    if(strcasecmp(method.c_str(), "get") == 0) {
      return true;
    }
    return false;
  }
  bool IsPost() {
    if(strcasecmp(method.c_str(), "post") == 0) {
      return true;
    }
    return false;
  }
  string GetParameter() {
    return parameter;
  }
  string GetRequestBody() {
    return request_body;
  }
  string GetMethod() {
    return method;
  }
}; 

class Response {
private:
  int code;
  string response_line;
  vector<string> response_header;
  string blank;
  string response_body;

public:
  Response()
    :code(200)
     ,blank("\r\n") 
  {

  }
  void SetCode(int c) {
    code = c;
  }
  int GetCode() {
    return code;
  }
  void SetResponseLine(string& line) {
    response_line = line;
  }
  string GetResponseLine() {
    return response_line;
  }
  vector<string>& GetResponseHeader() {
    return response_header;
  } 
  void SetResponseHeader(const string& header) {
    response_header.push_back(header);
  }
};

class Endpoint {
private:
  Request req;
  Response rsp;
  int sock;

private:
  void RecvRequestLine() {
    string line;
    Util::GetLine(sock, line);
    req.SetRequestLine(line); 
    req.RequestLinePause();
  }
  void RecvRequestHeader() {
    string line;  
    while(line != "\n") {
      line.clear();
      Util::GetLine(sock, line);
      if(line != "\n") {
        req.SetRequestHeader(line);
      }
    }
    req.RequestHeaderPause();
  }
  void RecvRequestBody() {
    int len = req.GetContentLength();
    string body;
    while(len--) {
      char c;
      if(recv(sock, &c, 1, 0) < 0) {
        LOG(Waring, "recv request_body error");
      }
      body.push_back(c);
    }
    req.SetRequestBody(body);

  }
  
  void MakeResponseLine() {
    string response_line;
    int code = rsp.GetCode();
    response_line += VERSION; 
    response_line += " ";
    response_line += code;
    response_line += " ";
    response_line += CodeToDesc(code);
    response_line += "\r\n";
    rsp.SetResponseLine(response_line);
  }
  void MakeResponseHeader() {
    string content_type = "Content-Type: ";
    string suffix = req.MakeSuffix();
    content_type += SuffixToDesc(suffix);
    content_type += "\r\n";
    rsp.SetResponseHeader(content_type);
    rsp.SetResponseHeader("\r\n");
  }
  void NonCgiExec() {
    //获取请求文件的大小
    ssize_t size = req.GetFileSize();
    int fd = open(req.GetPath().c_str(), O_RDONLY);
    if(fd < 0) {
      LOG(Error, "path is not exit bug");
      return;
    }
    //将客户端想访问的文件通过套接字发送
    sendfile(sock, fd, nullptr, size);
    close(fd);
  }
  void CgiExec() {
    //如果请求方法是get方法，通过环境变量给替换后的子进程传参
    //如果请求方法是post方法，通过管道给替换后的子进程传参
    //替换后的子进程通过管道向父进程返回执行后的结果
    //替换后的子进程如何看到匿名管道呢？ 父进程通过重定向，将pipe_in重定向到标准输入，将pipe_out重定向到标准输出
    //替换后的子进程使用cin时就是从pipe_in管道中读数据，使用cout就是往pipe_out中写数据
    int pipe_in[2];         //父进程通过该管道向子进程写数据
    int pipe_out[2];        //子进程通过该管道向父进程写数据
    pipe(pipe_in);
    pipe(pipe_out);
    string method_env = "METHOD_ENV=";
    method_env += req.GetMethod();
    putenv((char*)method_env.c_str());
    //要使替换后的进程能看到环境变量，必须将环境变量的字符变量定义为全局的，
    //因为putenv导入字符串变量为环境变量时，不会为该环境变量分配内存，而是使用的程序中定义变量的内存
    //是将该字符串的地址放在环境中，如果该变量是局部的，例如在下面的if中定义，就会导致
    //if退出后，该字符串被销毁，环境变量也不复存在，程序替换后就不能读到环境变量！
    string parameter_env;
    string content_length_env;
    if(req.IsGet()) {
      //是get方法，通过导入环境变量，使替换后的程序能读到参数
      parameter_env = "PARAMETER_ENV=";
      parameter_env += req.GetParameter();
      //LOG(Notice, parameter_env);
      putenv((char*)parameter_env.c_str());
      //string p = getenv("PARAMETER_ENV");
      //cout << p << endl;
    }
    else if(req.IsPost()) {
      //是post方法，参数在正文里，通过环境变量将content-length导给替换后的子进程
      content_length_env = "CONTENT-LENGTH=";
      content_length_env += to_string(req.GetContentLength());
      putenv((char*)content_length_env.c_str());
    }
    else {
      //其他方法，暂不处理
    }
    pid_t pid = fork();
    if(pid == 0) {
      //子进程
      close(pipe_in[1]);
      close(pipe_out[0]);
      //原来0号文件描述符指向的是标准输入，现在0号文件描述符指向pipe_in[0]指向的文件
      dup2(pipe_in[0], 0);
      //原来1号文件描述符指向的是标准输出，现在1号文件描述符指向pipe_out[1]指向的文件
      dup2(pipe_out[1], 1);
      //以上重定向工作完成后，被替换的子进程就可以使用cout和cin来从管道中读取和写入数据
      //从标准输入中读取，实际是从pipe_in[0]指向的文件中读取
      //向标准输出中输出，实际是向pipe_out[1]指向的文件中输出
      //程序替换
      string path = req.GetPath();
      //cout << path << endl;
      execl(path.c_str(), path.c_str(), nullptr);
      exit(1);
    }
    else if(pid < 0) {
      LOG(Fatal, "fork error");
      return;
    }
    else {
      //父进程，如果是post方法，把正文写入管道
      close(pipe_in[0]);
      close(pipe_out[1]);
      if(req.IsPost()) {
        string body = req.GetRequestBody();
        for(size_t i = 0; i < body.size(); ++i) {
          write(pipe_in[1], &body[i], 1); 
        }
      }
      //将cgi程序的运行结果从管道中读出来，发送给客户端
      ssize_t s = 0;
      do{
         char c;
         s = read(pipe_out[0], &c, 1);
         if(s > 0) {
           send(sock, &c, 1, 0);
         }
      }while(s > 0);
      waitpid(pid, nullptr, 0); 
    }


  }
public:
  Endpoint(int _sock) :sock(_sock){
  }  
  
  void RecvRequest() {
    //读取请求行
    RecvRequestLine();
    //读取报头
    RecvRequestHeader();
    //读取正文
    if(req.IsNeedRecvBody()) {
      RecvRequestBody();
    }
    
  }
  void PauseRequest() {
    //判断请求是否合法,只处理get和post方法
    if(!req.IsMethodLegal()) {
      rsp.SetCode(404);  
      LOG(Waring, "request method legal");
    }
    else {
      //分析uri中是否带参,如果带参就要分离参数，得到纯净的uri
      req.UriPause();
      //判断得到的路径是否需要加index.html
      req.IsAddHomePage();
      //判断当前路径下的文件是否存在
      if(req.IsPathLegal()) {
        //如果当前请求的是一个文件夹，给路径加上/index.html
        if(req.IsDir()) {
          string path = req.GetPath();
          path += '/';
          req.SetPath(path);
          req.IsAddHomePage();
        }
        //如果请求的是一个文件，则判断是否是可执行程序
        else {
          if(req.IsCgi()) {
            req.SetCgi(true);
          }
          else {
            //普通文件
          }
        }
      } 
      //文件不存在
      else {
        LOG(Waring, "request file not found");
        rsp.SetCode(404);
      }
    }
  }

  void MakeResponse() {
    MakeResponseLine();     
    MakeResponseHeader();
  }

  void SendResponse() {
    string line = rsp.GetResponseLine();
    //发送响应行
    send(sock, line.c_str(), line.size(), 0);
    vector<string> header = rsp.GetResponseHeader();
    //发送响应报头
    for(auto& s : header) {
      //cout << s << endl;
      send(sock, s.c_str(), s.size(), 0);
    }
    //发送正文 
    //访问的是可执行程序，就发送cgi程序的运行结果
    if(req.GetCgi()) {
      LOG(Notice, "use cgi model");
      CgiExec();
    }
    //访问的是静态网页，直接发送网页
    else {
      LOG(Notice, "use non cgi model");
      NonCgiExec();
    }

  }

  ~Endpoint() {
    LOG(Notice, "link break");
    close(sock);
  }

};



class Entry{
public:
  static void Service(int arg) {
    //int sock = *(int*)arg;
    int sock = arg;
#ifdef Text 
    char buf[2048] = {0};
    ssize_t s = recv(sock, buf, sizeof(buf), 0);
    if(s > 0) {
      buf[s] = 0;
      std::cout << buf << std::endl;
    }
    LOG(Notice, "link break");
    close(sock);
#else 
    Endpoint* ep = new Endpoint(sock);
    ep->RecvRequest();
    ep->PauseRequest();
    ep->MakeResponse();
    ep->SendResponse();
    delete ep;
#endif
  }

};
