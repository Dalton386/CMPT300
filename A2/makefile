CC = gcc 
CFLAGS = -O0

TARGET = assn2

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) $(CFLAGS) -o $@ $^
$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(TARGET).o
run: 
	./$(TARGET)



lab:
	make -f makefile.lab
lrun:
	./lab
lcln:
	rm -f lab lab.o