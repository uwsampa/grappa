
linked_list:
	gcc -O3 -fopenmp linked_list.c -o linked_list -lrt
	OMP_NUM_THREADS=4 ./linked_list
	objdump -S ./linked_list

.PHONY: linked_list