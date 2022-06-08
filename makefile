all: encrypt encrypt-driver.o encrypt-module.o clean

encrypt: encrypt-driver.o encrypt-module.o 
	gcc encrypt-driver.o encrypt-module.o -o encrypt

encrypt-driver.o: encrypt-driver.c encrypt-module.h 
	gcc -c encrypt-driver.c -lpthread -lrt

encrypt-module.o: encrypt-module.c encrypt-module.h
	gcc -c encrypt-module.c -lpthread -lrt
clean:
	rm -f *.o *~
