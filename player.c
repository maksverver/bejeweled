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

const size_t queue_cap = 10000;

static Move *best_move = NULL;  /* game trace for best score */
static int best_score = 0;      /* best possible score */

/* Precalculated candidate moves: */
static Candidate candidates[MAX_MOVES];
static int num_candidates;

/* Return the time in microseconds */
static long long ustime()
{
#ifdef TIME_SIM
    static long long counter = 0;
    counter += 1000;
    return counter;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000LL*tv.tv_sec + tv.tv_usec;
#endif
}

/* Merge nq into pq, freeing extra boards */
static void merge_board_queues(PriorityQueue *pq, PriorityQueue *nq)
{
    while (!pq_empty(nq))
    {
        if (pq_full(pq)) board_free(pq_pop_min(pq));
        pq_push(pq, pq_max_prio(nq), pq_max_data(nq));
        pq_pop_max(nq);
    }
}

static void search1(Game *game, long long max_usec)
{
    PriorityQueue *pq = pq_create(queue_cap);
    assert(pq != NULL);

    long long time_start = ustime();
    pq_push(pq, 0, board_clone(game->initial));
    while (!pq_empty(pq) && ustime() - time_start < max_usec)
    {
        Board *board = pq_pop_max(pq);
        if (board->score > best_score)
        {
            best_score = board->score;
            move_deref(best_move);
            best_move = move_ref(board->last_move);
        }
        if (board->moves == MOVE_LIMIT)
        {
            board_free(board);
            break;
        }

        int n;
        #pragma omp parallel for
        for (n = 0; n < num_candidates; ++n)
        {
            if (!move_valid_candidate(board, &candidates[n])) continue;

            int  r = candidates[n].r;
            int  c = candidates[n].c;
            bool v = candidates[n].vert;

            /* Build new board */
            Board *new_board = board_clone(board);
            assert(new_board != NULL);
            board_move(new_board, r, c, r + v, c + !v, 1);

            Board *old_board = NULL;
            #pragma omp critical
            {
                if (pq_full(pq)) old_board = pq_pop_min(pq);
                int prio = 10000*new_board->moves - r + new_board->score/100;
                pq_push(pq, prio, new_board);
            }
            board_free(old_board);
        }
        board_free(board);
    }
    if (pq_empty(pq)) printf("search1: Queue exhausted!\n");
    while (!pq_empty(pq)) board_free(pq_pop_min(pq));
    pq_destroy(pq);
}

/* Time-bounded search for optimal score. Does not work well on "hard" sets. */
static void search2(Game *game, long long max_usec)
{
    /* Priority queue for boards currently being processed */
    PriorityQueue *pq = pq_create(queue_cap);
    assert(pq != NULL);

    /* Queue for boards with moves == move_limit */
    PriorityQueue *nq = pq_create(queue_cap);
    assert(nq != NULL);

    /* Time limiting */
    long long time_start = ustime();
    long long next_update = 0;
    int move_limit = 1;
    int iterations = 0;

    pq_push(pq, 0, board_clone(game->initial));
    while (!pq_empty(pq) || !pq_empty(nq))
    {
        long long time_used = ustime() - time_start;
        if (time_used >= max_usec) break;
        ++iterations;

        if (pq_empty(pq))
        {
            /* Increase move limit because we need the new boards */
            ++move_limit;
            merge_board_queues(pq, nq);
        }
        else
        if (MOVE_LIMIT * time_used / max_usec > move_limit)
        {
            /* Increase move limit because time demands we make progress */
            move_limit = MOVE_LIMIT * time_used / max_usec;
            merge_board_queues(pq, nq);
        }

        /* Take next best board from the heap */
        Board *board = pq_pop_max(pq);

        if (board->score > best_score)
        {
            /* Update best score found */
            best_score = board->score;
            move_deref(best_move);
            best_move = move_ref(board->last_move);
        }

        if (next_update <= time_used)
        {
            printf(
                "iterations=%10d score=%10d moves=%5d pq_size=%5d nq_size=%5d "
                "move_limit=%6d score/move=%5d\n",
                iterations, board->score, board->moves,
                (int)pq_size(pq), (int)pq_size(nq),
                move_limit, board->score/(1 + board->moves) );
            next_update += 1000000; /* 1 sec */
        }

        if (board->moves >= MOVE_LIMIT || board->score >= SCORE_LIMIT)
        {
            printf("End of game reached!\n");
            board_free(board);
            break;
        }

        int n;
        #pragma omp parallel for
        for (n = 0; n < num_candidates; ++n)
        {
            if (!move_valid_candidate(board, &candidates[n])) continue;

            int  r = candidates[n].r;
            int  c = candidates[n].c;
            bool v = candidates[n].vert;

            /* Build new board */
            Board *new_board = board_clone(board);
            assert(new_board != NULL);
            board_move(new_board, r, c, r + v, c + !v, 1);

            int prio = new_board->score;

            Board *old_board = NULL;
            if (new_board->moves < move_limit)
            {
                /* Add to active queue */
                #pragma omp critical
                {
                    if (pq_full(pq)) old_board = pq_pop_min(pq);
                    pq_push(pq, prio, new_board);
                }
            }
            else
            {
                /* Add to next queue */
                #pragma omp critical
                {
                    if (pq_full(nq)) old_board = pq_pop_min(nq);
                    pq_push(nq, prio, new_board);
                }
            }
            board_free(old_board);
        }

        board_free(board);
    }

    if (pq_empty(pq)) printf("Queue exhausted.\n");

    /* Free queues */
    while (!pq_empty(pq)) board_free(pq_pop_min(pq));
    pq_destroy(pq);
    while (!pq_empty(nq)) board_free(pq_pop_min(nq));
    pq_destroy(nq);
}

#include <omp.h>

int main(int argc, char *argv[])
{
    long long time_start = ustime();
    long long time_limit = 1000000LL*(15*60 - 5); /* 14 minutes, 55 seconds */

    mem_debug_report_at_exit(stderr);

    if (argc > 2)
    {
        printf("Usage: player [<directory>]\n");
        return 0;
    }

    Game *game = game_load(argc < 2 ? "." : argv[1]);
    if (game == NULL)
    {
        perror("failed to load board definition");
        exit(1);
    }

    /* omp_set_num_threads(4); */
    printf("Using %d threads\n", omp_get_max_threads());

    /* Generate candidate moves */
    num_candidates = move_generate_candidates(game->initial, candidates);

    /* First, search for a single feasible solution */
    search1(game, time_limit);

    /* Search for maximum scoring solution */
    long long time_left = time_start + time_limit - ustime();
    if (time_left > 0) search2(game, time_left);

    game_free(game);

    /* Write best score trace */
    printf("Best score: %d\n", best_score);
    FILE *fp = fopen("uitvoer.txt", "wt");
    moves_print(best_move, fp);
    fclose(fp);

    move_deref(best_move);

    return 0;
}
