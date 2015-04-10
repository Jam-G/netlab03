all: client

.PHONY:all
client:client.c client.h function.c ../common/common.h
	gcc -Wall -pthread -o client client.c function.c client.h ../common/common.h
.PHONY:clean 
clean : 
	rm -rf *.o
	rm -rf client
