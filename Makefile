CFLAGS = -Wall -Wextra -Werror -Wconversion -pedantic
all: bench1 bench2 bench3 rr
vec.o: vec.c
	mpicc $(CFLAGS) -c -o $@ $^
bench1: bench1.c vec.o
	mpicc $(CFLAGS) -o $@ $^
bench2: bench2.c
	mpicc $(CFLAGS) -o $@ $^
bench3: bench3.c
	mpicc $(CFLAGS) -o $@ $^
rr:	rr.c
	mpicc $(CFLAGS) -o $@ $^