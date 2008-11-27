CFLAGS=-ansi -Wall -Wextra -g -O2 -m32 -march=i686 -DMEM_DEBUG
OBJS=Game.o Moves.o MemDebug.o

all: verifier player

clean:
	rm -f *.o

distclean: clean
	rm -f verifier player

verifier: verifier.c $(OBJS)
	$(CC) $(CFLAGS) -o verifier verifier.c $(OBJS)

player: player.c $(OBJS)
	$(CC) $(CFLAGS) -o player player.c $(OBJS)

.PHONY: all clean distclean
