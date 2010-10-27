


HEADERS=\
	node.h

run_linked_list: linked_list
	./linked_list 22 1

objdump_linked_list:
	objdump -S ./linked_list

linked_list: $(HEADERS) Makefile
	gcc -O3 -fopenmp linked_list.c -o linked_list -lrt

.PHONY: run_linked_list objdump_linked_list