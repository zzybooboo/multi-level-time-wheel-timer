gcc -g -fpic -c tw_timer.c
gcc -g -fpic -shared -o libtw_timer.so tw_timer.o 
gcc -g -c main.c 
gcc -g -o main main.o -ltw_timer -L .
