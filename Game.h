#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED


                        /* maximum score -- if you reach this, you win */
#define SCORE_LIMIT     (1000000000)

                        /* maximum moves -- if you reach this, you win */
#define MOVE_LIMIT      (100000)

                        /* maximum field height; larger fields are truncated */
#define MAX_HEIGHT      (50)

                        /* maximum field width; larger fields are truncated */
#define MAX_WIDTH       (50)


/* Fields are represented by a byte; -1 for blocked fields, 0 for empty fields,
   and 1 through 10 (inclusive) for blocks. */
typedef signed char Field;
#define FIELD_EMPTY     ((Field) 0)
#define FIELD_BLOCKED   ((Field)-1)

/* Represents the dynamic state of a game */
typedef struct Board
{
    struct Game *game;
    Field *fields;          /* fields in the board in row-major order */
    Field **drops;          /* array of pointers into the drop lists */
    int score;              /* total score so far */
    int moves;              /* total moves performed so far */
} Board;

/* Represents the static state of a game; i.e. the board dimensions,
   available pieces and drop lists. */
typedef struct Game
{
    int width, height;      /* rectangular size of the board */
    Field **drops_begin;    /* for each column, a pointer to begin of the list */
    Field **drops_end;      /* for each column, a pointer to the end of list */
    Board *initial;
} Game;

/* Macro to access fields in a game board; evaluates to an lvalue */
#define FLD(b,r,c) ((b)->fields[(r)*((b)->game->width) + (c)])

/* Macro to access the board width */
#define WID(b) ((b)->game->width)

/* Macro to access the board height */
#define HIG(b) ((b)->game->height)

/* Load a game definition from the given directory.

   If the game cannot be loaded for any reason (directory not found, missing
   files, invalid data, out of memory, et cetera) NULL is returned and errno
   gives an indication of what went wrong.

   The returned board must be freed with board_free. */
Game *game_load(const char *dir);

/* Free a board loaded with board_load. */
void game_free(Game *game);

/* Perform a move and return the score for this move; if this is zero, no
   scoring rows are formed and the move has not been executed.

   The squares indiciated by (r1,c1) and (r2,c2) must be distinct but adjacent.
*/
int board_move(Board *board, int r1, int c1, int r2, int c2);

/* Clone a board, or return NULL if memory allocation fails.
   The board returned must be freed with board_free(). */
Board *board_clone(Board *board);

/* Free a board. */
#define board_free(board) free(board)

/* For debugging: dump the board configuration in a human-readable format. */
void board_dump(Board *board, void *fp);

#endif /* ndef GAME_H_INCLUDED */
