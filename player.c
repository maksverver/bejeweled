#include "Game.h"
#include "Moves.h"
#include "MemDebug.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compares to boards according to higher score (primarily) or
   lower number of moves (secondarily). */
int cmp_board(const void *arg1, const void *arg2)
{
    const Board *b1 = arg1;
    const Board *b2 = arg2;
    if (b1->score == b2->score) return b1->score - b2->score;
    return b2->moves - b1->moves;
}

static void heap_swap(void **pq, size_t i, size_t j)
{
    void *tmp;

    tmp = pq[i];
    pq[i] = pq[j];
    pq[j] = tmp;
}

/* pq[0..size-1) is a valid max-heap;
   pq[size-1] is an element to be pushed into the heap. */
void heap_push(void **pq, size_t size, int (*cmp)(const void *, const void*))
{
    assert(size > 0);

    size_t cur, par;

    cur = size - 1;
    while (cur > 0)
    {
        par = cur/2;
        if (cmp(pq[par], pq[cur]) >= 0) break;

        /* Swap with parent */
        heap_swap(pq, cur, par);
        cur = par;
    }
}

/* pq[0..size) is a valid max-heap;
   afterwards, pq[0..size-1) is a valid max-heap and pq[size-1] is the formerly
   largest element in the heap. */
void heap_pop(void **pq, size_t size, int (*cmp)(const void *, const void*))
{
    assert(size > 0);

    size_t cur, child;

    /* Swap max-value to the end */
    heap_swap(pq, 0, --size);

    /* Restore heap property */
    cur = 0;
    while ((child = 2*cur + 1) < size)
    {
        /* Select child to compare against (largest of the two) */
        if (child + 1 < size && cmp(pq[child], pq[child + 1]) < 0) ++child;

        /* See if child is larger than current element */
        if (cmp(pq[child], pq[cur]) >= 0) break;

        /* Swap current node with largest child */
        heap_swap(pq, cur, child);
        cur = child;
    }
}

static void search(Game *game)
{
    void **pq = malloc(1024*sizeof(void*));
    size_t pq_cap = 1024, pq_size = 0;

    assert(pq != NULL);

    pq[pq_size++] = game->initial;
    heap_push(pq, pq_size, cmp_board);

    while (pq_size > 0)
    {
        heap_pop(pq, pq_size, cmp_board);
        Board *board = pq[--pq_size];

        printf("score=%10d moves=%10d\n", board->score, board->moves);

        if (board->moves == MOVE_LIMIT) continue;

        int r, c, v;
        for (v = 0; v < 2; ++v)
        {
            for (r = 0; r < HIG(board); ++r)
            {
                for (c = 0; c < WID(board); ++c)
                {
                    if (!move_valid(board, r, c, v)) continue;

                    /* Build new board */
                    Board *new_board = board_clone(board);
                    board_move(new_board, r, c, r + v, c + !v);

                    /* Resize queue if necessary */
                    if (pq_size == pq_cap)
                    {
                        pq_cap *= 2;
                        pq = realloc(pq, pq_cap*sizeof(void*));
                        assert(pq != NULL);
                    }
                    pq[pq_size++] = new_board;
                    heap_push(pq, pq_size, cmp_board);
                }
            }
        }

        board_free(board);
    }
    printf("Queue exhausted.\n");
}

int main(int argc, char *argv[])
{
    Game *game;

    mem_debug_report_at_exit(stderr);

    if (argc > 2)
    {
        printf("Usage: player [<directory>]\n");
        return 0;
    }

    game = game_load(argc < 2 ? "." : argv[1]);
    if (game == NULL)
    {
        perror("failed to load board definition");
        exit(1);
    }

    search(game);

    return 0;
}
