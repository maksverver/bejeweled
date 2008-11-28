#include "Game.h"
#include "MemDebug.h"
#include "Moves.h"
#include "PriorityQueue.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int score_per_move = 430;

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

/* TODO: add time limit to search? */
static int search(Game *game)
{
    PriorityQueue *pq = pq_create(1000, cmp_board);
    assert(pq != NULL);

    /* Keep track of best score so far, and corresponding best last move */
    int best_score = 0;
    int best_moves = 0;
    Move *best_move = NULL;

    pq_push(pq, board_clone(game->initial));

    long long iterations = 0;
    while (!pq_empty(pq))
    {
        ++iterations;

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

        if (iterations%10000 == 0)
        {
            printf( "iterations=%10lldk score=%10d moves=%10d pq_size=%10d\n",
                    iterations/1000, board->score, board->moves, pq_size(pq) );
        }

        if (board->moves >= MOVE_LIMIT || board->score >= SCORE_LIMIT)
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

                    /* Add to queue if necessary */
                    if (pq_full(pq)) board_free(pq_pop_max(pq));
                    pq_push(pq, new_board);
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

    /* Free priority queue */
    while (!pq_empty(pq)) board_free(pq_pop_min(pq));
    pq_destroy(pq);

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

    int pass;
    for (pass = 0; pass < 1; ++pass)
    {
        score_per_move = search(game);
    }

    game_free(game);

    return 0;
}
