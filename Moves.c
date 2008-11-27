#include "Moves.h"

static int scan_left(Board *b, int r, int c, int f)
{
    int d = c;
    while (d > 0 && FLD(b, r, d - 1) == f) --d;
    return c - d;
}

static int scan_right(Board *b, int r, int c, int f)
{
    int d = c;
    while (d + 1 < WID(b) && FLD(b, r, d + 1) == f) ++d;
    return d - c;
}

static int scan_up(Board *b, int r, int c, int f)
{
    int s = r;
    while (s > 0 && FLD(b, s - 1, c) == f) --s;
    return r - s;
}

static int scan_down(Board *b, int r, int c, int f)
{
    int s = r;
    while (s + 1 < HIG(b) && FLD(b, s + 1, c) == f) ++s;
    return s - r;
}

bool move_valid(Board *b, int r, int c, bool vert)
{
    int f, g;

    /* Check if move is out of bounds ((r,c) is assumed to be in bounds) */
    if (vert ? (r == HIG(b) - 1) : (c == WID(b) - 1)) return false;

    f = FLD(b, r, c);
    g = FLD(b, r + vert, c + !vert);

    /* Check if fields moved have valid and different colors */
    if (f == g || f <= 0 || g <= 0) return false;

    if (!vert)
    {
        return
            scan_left  (b, r, c, g) >= 2 ||
            scan_up    (b, r, c, g) +
            scan_down  (b, r, c, g) >= 2 ||

            scan_right (b, r, c + 1, f) >= 2 ||
            scan_up    (b, r, c + 1, f) +
            scan_down  (b, r, c + 1, f) >= 2;
    }
    else
    {
        return
            scan_up    (b, r, c, g) >= 2 ||
            scan_left  (b, r, c, g) +
            scan_right (b, r, c, g) >= 2 ||

            scan_down  (b, r + 1, c, f) >= 2 ||
            scan_left  (b, r + 1, c, f) +
            scan_right (b, r + 1, c, f) >= 2;
    }
    return false;
}
