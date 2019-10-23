all: project3 server

project3.o: project3.c 
	gcc -c project3.c  

project3: project3.o storage_remote.o
	gcc project2.o storage_remote.o -o project3

storage_remote.o: storage_remote.c storage_remote.h storage_common.h
	gcc -c storage_remote.c

server.o: server.c 
	gcc -c server.c

storage.o: storage.c storage.h
	gcc -c storage.c

server: server.o storage.o
	gcc server.o storage.o -o server

clean:
	rm -f *.o project3 server pipe_in pipe_out

zip: 
	zip project3.zip *.c *.h Makefile README.txt
