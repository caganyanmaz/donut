CC = gcc
INCLUDE = -lm
DEBUG_CFLAGS = -Wall -fprofile-arcs -ftest-coverage -D DEBUGGING
RELEASE_CFLAGS = -Wall -O3


debug:
	$(CC) donut.c $(INCLUDE) $(DEBUG_CFLAGS) -o debug

donut: donut.o
	$(CC) donut.o $(INCLUDE) $(RELEASE_CFLAGS) -o donut

donut.o: donut.c
	$(CC) -c donut.c $(RELEASE_CFLAGS) -o donut.o
