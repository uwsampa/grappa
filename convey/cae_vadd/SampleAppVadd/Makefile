EXEC = UserApp.exe

all:	$(EXEC)

clean:
	rm -f $(EXEC)
	make --directory=../sim clean

#$(EXEC):	UserApp.c cpTest1.s
#	cnycc -g UserApp.c cpTest1.s -o UserApp.exe

$(EXEC):	linked_list.c linked_list-alloc.c linked_list.s
	cnycc -g $^ -o $@
