#ifndef MOVES_H
#define MOVES_H

#include "Game.h"
#include <stdbool.h>

#define MAX_MOVES (2*MAX_WIDTH*MAX_HEIGHT - MAX_WIDTH - MAX_HEIGHT)

/* Models a move, consisting of two adjacent fields to be swapped.
   If vert == false, then these fields are (r,c) and (r,c+1);
   if vert == true,  then these fields are (r,c) and (r+1,c). */
typedef struct Candidate
{
    char r, c;
    bool vert;
} Candidate;

/* Returns whether the given move is valid.
   Caller must ensure that (r,c) is a valid field reference.

   This function returns false if the adjacent field is not on the board, or
   if swapping the fields does not score any rows. */
bool move_valid(const Board *b, int r, int c, bool vert);

/* Returns whether the given candidate move is valid.
   (A candidate move is a move which has already been verified to stay inside
    the grid, and not operate on a blocked cell; therefore, the only thing
    left to check is whether executing the move results in a positive score.)
*/
bool move_valid_candidate(const Board *b, Candidate *move);

/* Generates a list of candidate moves.
   `moves` must be an array of size MAX_MOVES.
   The number of candidates found is returned. */
int move_generate_candidates(const Board *b, Candidate *moves);

#endif /* ndef MOVES_H */
