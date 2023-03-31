FLAG=gcc -c -Wall -Werror -std=c89 -pedantic -D SO_BLOCK_SIZE=10 -D SO_REGISTRY_SIZE=1000

all:master user node libro_mastro

master:master.o funzioni_master.o
	gcc master.o funzioni_master.o -o master

user: user.o funzioni_user.o
	gcc user.o funzioni_user.o -o user
	
node: node.o funzioni_nodo.o
	gcc node.o funzioni_nodo.o -o node
	
libro_mastro: libro_mastro.o funzioni_mastro.o
	gcc libro_mastro.o funzioni_mastro.o  -o libro_mastro
	
master.o: master.c master.h
	$(FLAG) master.c
funzioni_master.o: funzioni_master.c master.h
	$(FLAG) funzioni_master.c
	
user.o: user.c user.h
	$(FLAG) user.c
funzioni_user.o: funzioni_user.c user.h
	$(FLAG) funzioni_user.c	
	
node.o: node.c node.h
	$(FLAG) node.c
funzioni_nodo.o: funzioni_nodo.c node.h
	$(FLAG) funzioni_nodo.c	
	
libro_mastro.o: libro_mastro.c libro_mastro.h
	$(FLAG) libro_mastro.c
funzioni_mastro.o: funzioni_mastro.c libro_mastro.h
	$(FLAG) funzioni_mastro.c	

clean:
	rm *.o  libro_mastro node user master

run:
	./master 20 10 20 10000 10000000 20000000 1 10 100
	make clean
	



