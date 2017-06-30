all: myclient myserver

myserver: myserver.c error.c error.h
	gcc -pthread -o ../bin/myserver myserver.c error.c -I. -ggdb

myclient: myclient.c error.c error.h
	gcc -pthread -o ../bin/myclient myclient.c error.c -I. -ggdb

clean:
	-rm -f ../bin/myclient ../bin/myserver
