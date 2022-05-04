CC=gcc
TARGET=proj2
FLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
CFLAGS=-pthread -lrt

$(TARGET) : $(TARGET).c
	$(CC) $(FLAGS) -o $(TARGET) $(TARGET).c $(CFLAGS)

clean:
	rm $(TARGET)
	rm $(TARGET).out

run:
	./$(TARGET) 100	100	30	30

defll: clean $(TARGET)  run
	sleep 1
	cat $(TARGET).out


test: $(TARGET).c
	./test.py
