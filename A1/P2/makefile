my_shell: my_shell.o
	gcc -o $@ $^
my_shell.o: my_shell.c
	gcc -c $<

run:
	./my_shell

clean:
	rm -f my_shell my_shell.o
