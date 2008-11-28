#include "Game.h"
#include "MemDebug.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    Game *game;
    Board *board;
    int moves;

    mem_debug_report_at_exit(stderr);

    if (argc > 2)
    {
        printf("Usage: verifier [<directory>]\n");
        return 0;
    }

    game = game_load(argc < 2 ? "." : argv[1]);
    if (game == NULL)
    {
        perror("failed to load board definition");
        exit(1);
    }
    board = board_clone(game->initial);

    for (moves = 0; moves < MOVE_LIMIT; ++moves)
    {
        int x1, y1, x2, y2;
        char dir;

        if (scanf("%d %d %ch", &x1, &y1, &dir) != 3)
        {
            printf("EOF reached.\n");
            break;
        }

        if (dir == 'N')
        {
            x2 = x1;
            y2 = y1 - 1;
        }
        else
        if (dir == 'O')
        {
            x2 = x1 + 1;
            y2 = y1;
        }
        else
        if (dir == 'Z')
        {
            x2 = x1;
            y2 = y1 + 1;
        }
        else
        if (dir == 'W')
        {
            x2 = x1 - 1;
            y2 = y1;
        }
        else
        {
            printf("Invalid direction (%c).\n", dir);
            break;
        }

        if ( x1 < 0 || x1 >= game->width || y1 < 0 || y1 >= game->height )
        {
            printf("Start position (%d,%d) out of bounds\n", x1, y1);
            break;
        }

        if ( x2 < 0 || x2 >= game->width || y2 < 0 || y2 >= game->height )
        {
            printf("Goal position (%d,%d) out of bounds\n", x2, y2);
            break;
        }

        if (board_move(board, y1, x1, y2, x2, 0) == 0)
        {
            board_dump(board, stdout);
            printf( "Move %d (%d %d %c) does not score any points (undone)\n",
                    1 + moves, x1, y1, dir);
        }
        else
        if (board->score >= SCORE_LIMIT)
        {
            printf("Maximum score limit reached. Congratulations!\n");
            break;
        }
    }

    if (board->score == SCORE_LIMIT)
    {
        printf("Infinite score after %d moves\n", moves);
    }
    else
    {
        printf("Total score %d after %d moves.\n", board->score, moves);
    }

    board_dump(board, stdout);
    board_free(board);
    game_free(game);

    return 0;
}
