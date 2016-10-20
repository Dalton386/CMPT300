proc_parse: proc_parse.o
	gcc -o $@ $^
proc_parse.o: proc_parse.c
	gcc -c $<

clean:
	rm -f proc_parse proc_parse.o
run: 
	./proc_parse
