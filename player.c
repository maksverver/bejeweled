#include "Game.h"
#include "Moves.h"
#include "MemDebug.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compares to boards and returns <0, 0, >0 when the first board is better than,
   equal to, or worse than the second board.

   In this context, a board is better than another board if the former has a
   higher score, or if the scores are equal, but the former used fewer moves.
*/
int cmp_board(const void *arg1, const void *arg2)
{
    const Board *b1 = arg1;
    const Board *b2 = arg2;
    if (b1->score != b2->score) return b2->score - b1->score;
    return b1->moves - b2->moves;
}

static void heap_swap(void **pq, size_t i, size_t j)
{
    void *tmp;

    tmp = pq[i];
    pq[i] = pq[j];
    pq[j] = tmp;
}

#if 0
void check_heap(void **pq, size_t size, int (*cmp)(const void *, const void*))
{
    size_t cur;
    for (cur = 1; cur < size; ++cur)
    {
        if (cmp(pq[(cur-1)/2], pq[cur]) > 0)
        {
            printf("heap violation at position %zd\n", cur);
            abort();
        }
    }
}
#endif

/* pq[0..size-1) is a valid min-heap;
   pq[size-1] is an element to be pushed into the heap. */
void heap_push(void **pq, size_t size, int (*cmp)(const void *, const void*))
{
    assert(size > 0);

    size_t cur, par;

    cur = size - 1;
    while (cur > 0)
    {
        par = (cur - 1)/2;

        /* Stop if parent is not greater than current node */
        if (cmp(pq[par], pq[cur]) <= 0) break;

        /* Swap with parent */
        heap_swap(pq, cur, par);
        cur = par;
    }
}

/* pq[0..size) is a valid min-heap;
   afterwards, pq[0..size-1) is a valid min-heap and pq[size-1] is the formerly
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
        /* Select child to compare against (smallest of the two) */
        if (child + 1 < size && cmp(pq[child + 1], pq[child]) < 0) ++child;

        /* Stop if current element is not greater than child */
        if (cmp(pq[cur], pq[child]) <= 0) break;

        /* Swap current node with smallest child */
        heap_swap(pq, cur, child);
        cur = child;
    }
}

static void search(Game *game)
{
    size_t pq_cap = 1000000, pq_size = 0;
    void **pq = malloc(pq_cap*sizeof(void*));

    assert(pq != NULL);

    pq[pq_size++] = board_clone(game->initial);
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

                    /* Truncate queue if necessary */
                    if (pq_size == pq_cap)
                    {
                        printf("Truncating queue...\n");
                        qsort((void*)pq, pq_size, sizeof(void*), cmp_board);
                        while (pq_size > pq_cap/2) board_free(pq[--pq_size]);
                    }
                    pq[pq_size++] = new_board;
                    heap_push(pq, pq_size, cmp_board);
                }
            }
        }

        board_free(board);
    }
    printf("Queue exhausted.\n");

    while (pq_size > 0) board_free(pq[--pq_size]);
    free(pq);
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

    game_free(game);

    return 0;
}
