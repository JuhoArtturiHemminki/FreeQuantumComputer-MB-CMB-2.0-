CC = gcc
CFLAGS = -O3 -march=native -pthread
TARGET = free_quantum_bench
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
