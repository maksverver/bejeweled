#ifndef MOVES_H
#define MOVES_H

#include "Game.h"
#include <stdbool.h>

#if 0   /* commented out because this stuff is not used currently */

#define MAX_MOVES (2*MAX_WIDTH*MAX_HEIGHT - MAX_WIDTH - MAX_HEIGHT)

/* Models a move, consisting of two adjacent fields to be swapped.
   If vert == false, then these fields are (r,c) and (r,c+1);
   if vert == true,  then these fields are (r,c) and (r+1,c). */
typedef struct Move
{
    int r, c;
    bool vert;
} Move;

#endif

/* Returns whether the given move is valid.
   Caller must ensure that (r,c) is a valid field reference.

   This function returns false if the adjacent field is not on the board, or
   if swapping the fields does not score any rows. */
bool move_valid(Board *b, int r, int c, bool vert);

#endif /* ndef MOVES_H */
