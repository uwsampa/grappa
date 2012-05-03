#
# $Id: test_allocator.make,v 1.1 2001/10/21 22:09:34 cristiv Exp cristiv $
#
SRCDIR=	../Source/
PROG=	test_allocator
SRC=	allocator.cpp main.cpp
OBJS=	$(SRC:.cpp=.o)

%.o:$(SRCDIR)%.cpp
	$(CXX) -c $(CXXFLAGS) $<

SOURCE=$(addprefix $(SRCDIR), $(SRC))

$(PROG): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJS): $(SOURCE) Makefile test_allocator.make

clean:
	rm -f $(OBJS) $(PROG)

include depend.make
