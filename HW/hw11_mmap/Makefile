SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %, $(SRC))
CFLAGS+=-lm

all:$(OBJ)

%:%.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm -rf $(OBJ) *.o