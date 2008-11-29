CFLAGS=-ansi -Wall -Wextra -g -O3 -m32 -march=i686 -DMEM_DEBUG
SRCS=Game.c MemDebug.c Moves.c PriorityQueue.c
OBJS=Game.o MemDebug.o Moves.o PriorityQueue.o

all: verifier player

clean:
	rm -f *.o

distclean: clean
	rm -f verifier player

verifier: verifier.c $(OBJS)
	$(CC) $(CFLAGS) -o verifier verifier.c $(OBJS)

player: player.c $(SRCS)
	$(CC) $(CFLAGS) -fwhole-program -combine -o player player.c $(SRCS)

.PHONY: all clean distclean
