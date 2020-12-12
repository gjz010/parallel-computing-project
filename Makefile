CFLAGS = -Wall -Werror -Wconversion -pedantic -O3 -std=c99
CXXFLAGS = -Wall -Werror -Wconversion -pedantic -O3
all: bench1 bench2 bench3 rr rr_dist/Reduction rr_dist/Validation_mpi RandomGen
vec.o: vec.c
	mpicc $(CFLAGS) -c -o $@ $^
utils.o: utils.c
	mpicc $(CFLAGS) -c -o $@ $^
bench1: bench1.c vec.o utils.o
	mpicc $(CFLAGS) -o $@ $^
bench2: bench2.c vec.o utils.o
	mpicc $(CFLAGS) -o $@ $^
bench3: bench3.c utils.o vec.o
	mpicc $(CFLAGS) -o $@ $^
rr_io.o: rr_io.cpp
	mpic++ $(CXXFLAGS) -c -o $@ $^
rr.o: rr.c
	mpicc $(CFLAGS) -c -o $@ $^
rr:	rr.o utils.o rr_io.o
	mpic++ $(CXXFLAGS) -o $@ $^
rr_dist/Reduction: rr_dist/Reduction.cpp
	g++ -O3 -o $@ $^
rr_dist/Validation_mpi: rr_dist/Validation_mpi.cpp
	g++ -O3 -o $@ $^
RandomGen: RandomGen.c
	gcc $(CFLAGS) -o $@ $^
rr_test:
	./rr_test.sh
clean:
	rm bench1 bench2 bench3 rr vec.o utils.o rr_io.o rr.o rr_dist/Reduction rr_dist/Validation_mpi RandomGen -f
clean_mat:
	rm *.mat matrix bench1.out -f
distclean: clean clean_mat