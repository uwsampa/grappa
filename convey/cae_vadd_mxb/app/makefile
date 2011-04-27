EXEC = UserApp.exe

all:	$(EXEC)
	make --directory=../CaeSimPers

clean:
	rm -f $(EXEC)
	make --directory=../CaeSimPers clean

$(EXEC):	UserApp.c cpTest1.s
	cnycc -g UserApp.c cpTest1.s -o UserApp.exe

