all: project2 server

project2.o: project2.c 
	gcc -c project2.c  

project2: project2.o storage_remote.o
	gcc project2.o storage_remote.o -o project2

storage_remote.o: storage_remote.c storage_remote.h storage_common.h
	gcc -c storage_remote.c

server.o: server.c 
	gcc -c server.c

storage.o: storage.c storage.h
	gcc -c storage.c

server: server.o storage.o
	gcc server.o storage.o -o server

pipes: pipe_in pipe_out

pipe_in: 
	mkfifo pipe_in

pipe_out: 
	mkfifo pipe_out

clean:
	rm -f *.o project2 server pipe_in pipe_out

zip: 
	zip project2.zip *.c *.h Makefile README.txt
