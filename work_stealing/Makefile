CC=g++
LD=g++
CFLAGS= -g -fopenmp


%.o: %.cpp
	$(CC) $(CFLAGS) $< -c -o $@

workTest: workTest.o 
	$(LD) $(CFLAGS) $^ -o $@
	
clean:
	rm -f ./*.o ./workTest 
