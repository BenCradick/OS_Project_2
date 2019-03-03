CC = gcc
CFLAGS = -std=gnu99
OBJ = oss child
ALL: $(OBJ)
$(OBJ): %: %.c
	$(CC) $(CFLAGS) -o $@ $<
clean:
	/bin/rm $(OBJ)