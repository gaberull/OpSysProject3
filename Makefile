headers = oufs_lib.h oufs.h oufs_lib_support.h oufs.h storage.h virtual_disk.h
objects = oufs_format.o oufs_inspect.o oufs_mkdir.o oufs_ls.o oufs_rmdir.o oufs_stats.o 

all: oufs_format oufs_inspect oufs_mkdir oufs_ls oufs_rmdir oufs_stats

oufs_format.o: $(headers) oufs_format.c	#is it okay to put too many dependencies?
	gcc -c oufs_format.c

oufs_inspect.o: $(headers) oufs_inspect.c oufs_lib_support.h oufs_lib.h
	gcc -c oufs_inspect.c

oufs_ls.o: $(headers) oufs_ls.c
	gcc -c oufs_ls.c

oufs_mkdir.o: $(headers) oufs_mkdir.c
	gcc -c oufs_mkdir.c

oufs_rmdir.o: $(headers) oufs_rmdir.c
	gcc -c oufs_rmdir.c

oufs_stats.o: $(headers) oufs_stats.c
	gcc -c oufs_stats.c

oufs_format: $(objects)
	gcc oufs_format.o -o oufs_format

oufs_inspect: $(objects)
	gcc oufs_inspect.o -o oufs_inspect

oufs_ls: $(objects)
	gcc oufs_ls.o -o oufs_ls

oufs_mkdir: $(objects)
	gcc oufs_mkdir.o -o oufs_mkdir

oufs_rmdir: $(objects)
	gcc oufs_rmdir.o -o oufs_rmdir

oufs_stats: $(objects)
	gcc oufs_stats.o -o oufs_stats

clean:
	rm -f *.o oufs_format oufs_inspect oufs_mkdir oufs_rmdir oufs_stats

zip: 
	zip project3.zip *.c *.h Makefile README.txt
