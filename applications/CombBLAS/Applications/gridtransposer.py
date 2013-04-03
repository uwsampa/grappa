import sys 
filename = "MPICH_RANK_ORDER"
file = open(filename, "w")
print "Printing --transposed-- virtual mapping to MPI ranks to MPICH_RANK_ORDER"
print "The processor grid dimensions are ", sys.argv[1], "-by-", sys.argv[1]

dim = int(sys.argv[1]);
gen = range(dim);	
for i in range(dim):
	line = [x * dim + i for x in gen]
	for item in line:
		file.write(str(item)+ ',')
	file.write('\n');
file.close();
	
