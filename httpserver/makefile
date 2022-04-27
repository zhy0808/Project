#CURR_PATH=$(shell pwd)执行shell命令，将结果保存在前面的变量里，这里是获取绝对路径
.PHONY:all
all:server CGI

server:main.cpp
	g++ -o server main.cpp -std=c++11 -lpthread #-DText
.PHONY:CGI
CGI:
	#cd $(CURR_PATH)/cgi; make; cd $(CURR_PATH)
	cd ./cgi; make; cd ..

.PHONY:output
output:
	make clean;\
	make;\
	mkdir -p output/wwwroot;\
	cp server output;\
	cp -rf wwwroot/* output/wwwroot;\
	cp ./cgi/textcgi output/wwwroot;

.PHONY:clean
clean:
	rm server; cd ./cgi; make clean; cd ..;\
	rm -rf output
