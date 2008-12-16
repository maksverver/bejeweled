#include "Moves.h"

static int scan_left(const Board *b, int r, int c, int f)
{
    int d = c;
    while (d > 0 && FLD(b, r, d - 1) == f) --d;
    return c - d;
}

static int scan_right(const Board *b, int r, int c, int f)
{
    int d = c;
    while (d + 1 < WID(b) && FLD(b, r, d + 1) == f) ++d;
    return d - c;
}

static int scan_up(const Board *b, int r, int c, int f)
{
    int s = r;
    while (s > 0 && FLD(b, s - 1, c) == f) --s;
    return r - s;
}

static int scan_down(const Board *b, int r, int c, int f)
{
    int s = r;
    while (s + 1 < HIG(b) && FLD(b, s + 1, c) == f) ++s;
    return s - r;
}

static bool valid_horizontal(const Board *b, int r, int c)
{
    int f = FLD(b, r, c), g = FLD(b, r, c + 1);
    if (f == g) return false;   /* NB. not necessary but might save time */

    return
        scan_left  (b, r, c, g) >= 2 ||
        scan_up    (b, r, c, g) +
        scan_down  (b, r, c, g) >= 2 ||

        scan_right (b, r, c + 1, f) >= 2 ||
        scan_up    (b, r, c + 1, f) +
        scan_down  (b, r, c + 1, f) >= 2;
}

static bool valid_vertical(const Board *b, int r, int c)
{
    int f = FLD(b, r, c), g = FLD(b, r + 1, c);
    if (f == g) return false;   /* NB. not necessary but might save time */

    return
        scan_up    (b, r, c, g) >= 2 ||
        scan_left  (b, r, c, g) +
        scan_right (b, r, c, g) >= 2 ||

        scan_down  (b, r + 1, c, f) >= 2 ||
        scan_left  (b, r + 1, c, f) +
        scan_right (b, r + 1, c, f) >= 2;
}

bool valid_candidate(const Board *b, int r, int c, bool vert)
{
    /* Check if move is out of bounds ((r,c) is assumed to be in bounds) */
    if (vert ? (r == HIG(b) - 1) : (c == WID(b) - 1)) return false;

    /* Check if fields moved are free */
    int f = FLD(b, r, c), g = FLD(b, r + vert, c + !vert);
    if (f <= 0 || g <= 0) return false;

    return true;
}

bool move_valid(const Board *b, int r, int c, bool vert)
{
    if (!valid_candidate(b, r, c, vert)) return false;

    return vert ? valid_vertical(b, r, c)
                : valid_horizontal(b, r, c);
}

bool move_valid_candidate(const Board *b, Candidate *move)
{
    return move->vert ? valid_vertical(b, move->r, move->c)
                      : valid_horizontal(b, move->r, move->c);
}

int move_generate_candidates(const Board *b, Candidate *moves)
{
    int r, c, v, n = 0;
    for (v = 0; v < 2; ++v)
    {
        for (c = 0; c < WID(b); ++c)
        {
            for (r = 0; r < HIG(b); ++r)
            {
                if (valid_candidate(b, r, c, v))
                {
                    moves[n].r = r;
                    moves[n].c = c;
                    moves[n].vert = v;
                    n += 1;
                }
            }
        }
    }
    return n;
}
