#include "Game.h"
#include "MemDebug.h"
#include "Moves.h"
#include "PriorityQueue.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>   /* gettimeofday() */

/* Return the time in microseconds */
static long long ustime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000LL*tv.tv_sec + tv.tv_usec;
}

/* Compares to boards and returns <0, 0, >0 when the first board is better than,
   equal to, or worse than the second board.
*/
static int cmp_board(const void *arg1, const void *arg2)
{
    const Board *board1 = arg1, *board2 = arg2;
    /* Order by decreasing scores, or increasing moves if tied. */
    return (board2->score - board1->score) +
           (board1->moves - board2->moves);
}

/* Merge nq into pq, freeing extra boards */
static void merge_board_queues(PriorityQueue *pq, PriorityQueue *nq)
{
    while (!pq_empty(nq))
    {
        if (pq_full(pq)) board_free(pq_pop_max(pq));
        pq_push(pq, pq_pop_min(nq));
    }
}

static int search(Game *game, int max_secs)
{
    const size_t queue_cap = 1000;

    /* Priority queue for boards currently being processed */
    PriorityQueue *pq = pq_create(queue_cap, cmp_board);
    assert(pq != NULL);

    /* Queue for boards with moves == move_limit */
    PriorityQueue *nq = pq_create(queue_cap, cmp_board);
    assert(nq != NULL);

    /* Keep track of best score so far, and corresponding best last move */
    int best_score = 0;
    int best_moves = 0;
    Move *best_move = NULL;

    /* Time limiting */
    long long time_start = ustime();
    long long time_limit = 1000000LL*max_secs;    /* seconds -> microseconds */
    long long next_update = 0;
    int move_limit = 1;
    int iterations = 0;

    pq_push(pq, board_clone(game->initial));
    while (!pq_empty(pq) || !pq_empty(nq))
    {
        ++iterations;
        /* long long time_used = ustime() - time_start; */
        /* DEBUG: assume 200 microseconds per iteration */
        long long time_used = 200LL*iterations;

        if (pq_empty(pq))
        {
            /* Increase move limit because we need the new boards */
            ++move_limit;
            merge_board_queues(pq, nq);
        }
        else
        if (MOVE_LIMIT * time_used / time_limit > move_limit)
        {
            /* Increase move limit because time demands we make progress */
            move_limit = MOVE_LIMIT * time_used / time_limit;
            merge_board_queues(pq, nq);
        }

        /* Take next best board from the heap */
        Board *board = pq_pop_min(pq);

        if (board->score > best_score)
        {
            /* Update best score found */
            best_score = board->score;
            best_moves = board->moves;
            move_deref(best_move);
            best_move = board->last_move;
            move_ref(best_move);
        }

        if (next_update <= time_used)
        {
            printf(
                "iterations=%10d score=%10d moves=%5d pq_size=%5d nq_size=%5d "
                "move_limit=%6d est. score: %.3f\n",
                iterations, board->score, board->moves, pq_size(pq), pq_size(nq),
                move_limit, 1e-6*best_score/best_moves*MOVE_LIMIT);
            next_update += 1000000; /* 1 sec */
        }

                                /* DEBUG: */
        if (board->moves >= MOVE_LIMIT/50 || board->score >= SCORE_LIMIT)
        {
            printf("End of game reached!\n");
            board_free(board);
            break;
        }

        int r, c, v;
        for (v = 0; v < 2; ++v)
        {
            for (c = WID(board) - 1; c >= 0; --c)
            {
                for (r = HIG(board) - 1; r >= 0; --r)
                {
                    if (!move_valid(board, r, c, v)) continue;

                    /* Build new board */
                    Board *new_board = board_clone(board);
                    assert(new_board != NULL);
                    board_move(new_board, r, c, r + v, c + !v, 1);

                    if (new_board->moves < move_limit)
                    {
                        /* Add to active queue */
                        if (pq_full(pq)) board_free(pq_pop_max(pq));
                        pq_push(pq, new_board);
                    }
                    else
                    {
                        /* Add to next queue */
                        if (pq_full(nq)) board_free(pq_pop_max(nq));
                        pq_push(nq, new_board);
                    }
                }
            }
        }

        board_free(board);
    }

    if (pq_empty(pq)) printf("Queue exhausted.\n");

    /* Write best moves so far */
    printf("Best score: %d\n", best_score);
    {
        FILE *fp = fopen("uitvoer.txt", "wt");
        moves_print(best_move, fp);
        fclose(fp);
        move_deref(best_move);
    }

    /* Free queues */
    while (!pq_empty(pq)) board_free(pq_pop_min(pq));
    pq_destroy(pq);
    while (!pq_empty(nq)) board_free(pq_pop_min(nq));
    pq_destroy(nq);

    return best_moves > 0 ? best_score/best_moves : 0;
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

    search(game, 15*60);

    game_free(game);

    return 0;
}
