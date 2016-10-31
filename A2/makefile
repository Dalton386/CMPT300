CC =gcc
CFLAGS =

TARGET = assn2

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) -pthread -O0 -o $@ $^
$(TARGET).o: $(TARGET).c
	$(CC) -O0 -c $<

clean:
	rm -f $(TARGET) $(TARGET).o
run: 
	./$(TARGET)

# lab:
# 	make -f makefile.lab
# lrun:
# 	./lab
# lcln:
# 	rm -f lab lab.o