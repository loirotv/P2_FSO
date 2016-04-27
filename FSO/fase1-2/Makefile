all : winsuport.o tennis2

tennis2 : tennis2.c winsuport.o winsuport.h
	gcc tennis2.c winsuport.o -o tennis2 -lcurses -lpthread

winsuport.o : winsuport.c winsuport.h
	gcc -c winsuport.c -o winsuport.o

clean: 
	rm winsuport.o tennis0
