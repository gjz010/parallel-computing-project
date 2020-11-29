CFLAGS = -Wall -Wextra -Werror -Wconversion -pedantic -O3
all: bench1 bench2 bench3 rr
vec.o: vec.c
	mpicc $(CFLAGS) -c -o $@ $^
utils.o: utils.c
	mpicc $(CFLAGS) -c -o $@ $^
bench1: bench1.c vec.o utils.o
	mpicc $(CFLAGS) -o $@ $^
bench2: bench2.c vec.o utils.o
	mpicc $(CFLAGS) -o $@ $^
bench3: bench3.c
	mpicc $(CFLAGS) -o $@ $^
rr:	rr.c
	mpicc $(CFLAGS) -o $@ $^
clean:
	rm bench1 bench2 bench3 rr vec.o utils.o -f