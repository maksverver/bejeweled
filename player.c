#include "Game.h"
#include "Moves.h"
#include "MemDebug.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int score_per_move = 0;

static int expected_score(const Board *b)
{
    return b->score + score_per_move*(MOVE_LIMIT - b->moves);
}

/* Compares to boards and returns <0, 0, >0 when the first board is better than,
   equal to, or worse than the second board.
*/
static int cmp_board(const void *arg1, const void *arg2)
{
    return expected_score((const Board*)arg2) -
           expected_score((const Board*)arg1);
}

/* Indirect board comparison (used by qsort) */
static int cmp_board_indirect(const void *arg1, const void *arg2)
{
    return cmp_board(*(const void**)arg1, *(const void**)arg2);
}

static void heap_swap(void **pq, size_t i, size_t j)
{
    void *tmp;

    tmp = pq[i];
    pq[i] = pq[j];
    pq[j] = tmp;
}

#if 0
static void check_heap(void **pq, size_t size, int (*cmp)(const void *, const void*))
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
static void heap_push(void **pq, size_t size, int (*cmp)(const void *, const void*))
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
static void heap_pop(void **pq, size_t size, int (*cmp)(const void *, const void*))
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

__attribute__((__noinline__))   /* for profiling */
static void truncate_queue(void **pq, size_t *size, size_t cap)
{
    qsort( (void*)pq, *size, sizeof(void*), cmp_board_indirect );
    while (*size > cap/2) board_free(pq[--*size]);
}

/* TODO: add time limit to search */
static int search(Game *game)
{
    /* Priority queue of states */
    size_t pq_cap = 1000, pq_size = 0;
    void **pq = malloc(pq_cap*sizeof(void*));

    /* Keep track of best score so far, and corresponding best last move */
    int best_score = 0;
    int best_moves = 0;
    Move *best_move = NULL;

    assert(pq != NULL);

    pq[pq_size++] = board_clone(game->initial);
    heap_push(pq, pq_size, cmp_board);

    long long iterations = 0;
    while (pq_size > 0)
    {
        ++iterations;
        /* if (iterations == 50000) break; */

        /* Take next best board from the heap */
        heap_pop(pq, pq_size, cmp_board);
        Board *board = pq[--pq_size];

        if (board->score > best_score)
        {
            /* Update best score found */
            best_score = board->score;
            best_moves = board->moves;
            move_deref(best_move);
            best_move = board->last_move;
            move_ref(best_move);
        }

        if (iterations%10000 == 0)
        {
            printf( "iterations=%10lldk score=%10d moves=%10d pq_size=%10d\n",
                    iterations/1000, board->score, board->moves, pq_size );
        }

        if (board->moves == MOVE_LIMIT)
        {
            printf("End of game reached!\n");
            board_free(board);
            break;
        }

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
                    assert(new_board != NULL);
                    board_move(new_board, r, c, r + v, c + !v, 1);

                    /* Truncate queue if necessary */
                    if (pq_size == pq_cap) truncate_queue(pq, &pq_size, pq_cap);

                    pq[pq_size++] = new_board;
                    heap_push(pq, pq_size, cmp_board);
                }
            }
        }

        board_free(board);
    }

    if (pq_size == 0) printf("Queue exhausted.\n");

    /* Write best moves so far */
    printf("Best score: %d\n", best_score);
    {
        FILE *fp = fopen("uitvoer.txt", "wt");
        moves_print(best_move, fp);
        fclose(fp);
        move_deref(best_move);
    }

    /* Free priority queue */
    while (pq_size > 0) board_free(pq[--pq_size]);
    free(pq);

    return best_score/best_moves;
}

/* For setrlimit */
#include <sys/resource.h>

int main(int argc, char *argv[])
{
    Game *game;

    /* For debugging: limit available memory to 700MB */
    struct rlimit rlim = { 700*1024*1024, RLIM_INFINITY };
    setrlimit(RLIMIT_AS, &rlim);

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

    int pass;
    for (pass = 0; pass < 5; ++pass)
    {
        score_per_move = search(game);
    }

    game_free(game);

    return 0;
}
