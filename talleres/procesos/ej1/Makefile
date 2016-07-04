SRC=main.c
EXE=main
CC=gcc
CFLAGS= -std=gnu99

.PHONY: all clean

all: $(EXE)

$(EXE): $(SRC)
	$(CC) $(SRC) -o $(EXE) $(CFLAGS)
clean:
	rm $(EXE)
