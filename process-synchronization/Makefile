.RECIPEPREFIX=-
CC=gcc
CFLAGS=-std=gnu99 -Wall -Werror -Wextra -pedantic
LFLAGS= -pthread
EXECUTABLE=proj2
OBJ_FILE=proj2.o

.PHONY: clean zip all

all:$(EXECUTABLE)

%.o:%.c proj2.h
- $(CC) $(CFLAGS) -c $<

$(EXECUTABLE):$(OBJ_FILE)
- $(CC) $(LFLAGS) $< -o $@

clean:
- rm *.o proj2.out $(EXECUTABLE)

zip:
- zip proj2 *.c *.h Makefile
