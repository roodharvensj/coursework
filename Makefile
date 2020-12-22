CFLAGS=-Wall -pedantic -Wuninitialized -g
mycopyfile: mycopyfile.o myio.o
	gcc -o $@ $^
%.o: %.c
	gcc $(CFLAGS) -c $^
.PHONY: clean
clean:
	rm -f mycopyfile *.o