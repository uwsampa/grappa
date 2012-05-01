#
# $Id: test_threads.make,v 1.1 2001/10/21 22:09:34 cristiv Exp cristiv $
#
SRCDIR=	../Source/
PROG=	test_threads
SRC=	allocator.cpp TestThreads.cpp
OBJS=	$(SRC:.cpp=.o)

CXXFLAGS+=-D_MT


%.o:$(SRCDIR)%.cpp
	$(CXX) -c $(CXXFLAGS) $<

SOURCE=$(addprefix $(SRCDIR), $(SRC))

$(PROG): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJS): $(SOURCE)

clean:
	rm -f $(OBJS) $(PROG)

include depend.make
