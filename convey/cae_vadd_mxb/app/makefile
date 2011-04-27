EXEC = UserApp.exe

CFLAGS+=-std=gnu99 -g
LIBS+=-lrt

all:	$(EXEC)
	make --directory=../CaeSimPers

clean:
	rm -f $(EXEC)
	make --directory=../CaeSimPers clean

#$(EXEC):	UserApp.c cpTest1.s
#	cnycc -g UserApp.c cpTest1.s -o UserApp.exe

$(EXEC): linked_list.c linked_list-alloc.c linked_list1.s
	cnycc $(CFLAGS) $^ -o $@ $(LIBS)

