all: server

.PHONY:all
server:server.c server.h ../common/common.h
	gcc -Wall -pthread -o server server.c server.h ../common/common.h
.PHONY:clean 
clean : 
	rm -rf *.o
	rm -rf server
