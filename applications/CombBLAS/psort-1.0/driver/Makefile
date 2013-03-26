include ./Make.inc

psort: psort_driver.cc
	$(CXX) $(CXXFLAGS) -I../src psort_driver.cc -o psort $(LIBS)
	strip psort

clean:
	rm -f *.o psort results

