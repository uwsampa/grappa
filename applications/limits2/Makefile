


HEADERS=\
	node.h \
	ExperimentRunner.hpp

run_linked_list: linked_list
	./linked_list 22 1

objdump_linked_list:
	objdump -S ./linked_list

linked_list: $(HEADERS) Makefile linked_list.c
	g++ -O2 -fopenmp linked_list.c -o linked_list -lrt ../../../exper_runner/ExperimentRunner.o

.PHONY: run_linked_list objdump_linked_list
