/* TODO: calculate height of columns, and use that to speed up search
         for matching pairs and for filling up column (avoiding blocked
         cells entirely)? */

#include "Game.h"
#include "MemDebug.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct Rect
{
    int r1, c1, r2, c2;
} Rect;

static int min(int i, int j) { return i < j ? i : j; }
static int max(int i, int j) { return i > j ? i : j; }

/* Find an identifier for group i (using the stack) */
static int find(int *grp, int i)
{
    if (grp[i] == i) return i;
    return grp[i] = find(grp, grp[i]);
}

/* Unify groups i and j */
static void unify(int *grp, int *crd, int i, int j)
{
    int u = find(grp, i), v = find(grp, j);
    if (u == v) return;

    if (crd[u] >= crd[v])
    {
        crd[u] += crd[v];
        grp[v] = u;
    }
    else
    {
        crd[v] += crd[u];
        grp[u] = v;
    }
}

/* Removes groups of blocks that form scoring rows (i.e. at least three
   horizontally or vertically adjecent blocks)

   Uses a disjoint-set data structure to identify overlapping groups. */
static int remove_groups(Board *board, Rect *area)
{
    int n, r, c, score;
    int grp[MAX_HEIGHT*MAX_WIDTH];  /* group assignment for cells */
    int crd[MAX_HEIGHT*MAX_WIDTH];  /* group cardinality for cells */
    const int stride = board->game->width;
    const int w = area->c2 - area->c1;
    const int h = area->r2 - area->r1;
    Field (* fields)[stride]; /* HACK: GCC extension */

    fields = (Field(*)[stride])&FLD(board, area->r1, area->c1);

    /* Create initial groups: each cell in its own group */
    for (n = 0; n < w*h; ++n)
    {
        grp[n] = n;
        crd[n] = 1;
    }

    score = 0;

    /* Find horizontal groups of length at least 3 */
    for (r = 0; r < h; ++r)
    {
        for (c = 2; c < w; ++c)
        {
            if (fields[r][c] <= FIELD_EMPTY) continue;

            if ( fields[r][c - 2] == fields[r][c - 1] &&
                 fields[r][c - 1] == fields[r][c - 0] )
            {
                unify(grp, crd, w*r + c - 2, w*r + c - 1);
                unify(grp, crd, w*r + c - 1, w*r + c - 0);
                ++score;
            }
        }
    }

    /* Find vertical groups of length at least 3 */
    for (r = 2; r < h; ++r)
    {
        for (c = 0; c < w; ++c)
        {
            if (fields[r][c] <= FIELD_EMPTY) continue;

            if ( fields[r - 2][c] == fields[r - 1][c] &&
                 fields[r - 1][c] == fields[r - 0][c] )
            {
                unify(grp, crd, w*(r - 2) + c, w*(r - 1) + c);
                unify(grp, crd, w*(r - 1) + c, w*(r - 0) + c);
                ++score;
            }
        }
    }

    if (!score) return 0;

    /* Determine scores and remove marked fields.
       Groups of size 3, 4, 5+ are worth 50, 100, 250 points respectively. */
    score = 0;
    for (r = 0; r < h; ++r)
    {
        for (c = 0; c < w; ++c)
        {
            int u = find(grp, w*r + c);
            if (crd[u] < 3) continue;

            if (u == w*r + c)
            {
                if (crd[u] == 3)
                {
                    score += 50;
                }
                else
                if (crd[u] == 4)
                {
                    score += 100;
                }
                else /* crd[u] >= 5 */
                {
                    score += 250;
                }
            }
            fields[r][c] = FIELD_EMPTY;
        }
    }

    return score;
}

/* Compact columns (filling empty fields) and fill them up at the top with
   dropped blocks. */
static void fill_columns(Board *board, Rect *area)
{
    int c, r1, r2;
    Rect new_area = { 0, WID(board) - 1, 0, 0 };
    for (c = area->c1; c < area->c2; ++c)
    {
        /* Find first empty spot */
        for (r2 = area->r2 - 1; r2 >= area->r1; --r2)
        {
            if (FLD(board, r2, c) == FIELD_EMPTY) break;
        }

        if (r2 < area->r1) continue; /* no empty spots in this column */

        /* Expand new area */
        if (c < new_area.c1) new_area.c1 = c;
        if (c >= new_area.c2) new_area.c2 = c + 1;
        if (r2 >= new_area.r2) new_area.r2 = r2 + 1;

        /* Drop down fields on empty spot */
        for (r1 = r2 - 1; r1 >= 0; --r1)
        {
            if (FLD(board, r1, c) != FIELD_EMPTY)
            {
                FLD(board, r2, c) = FLD(board, r1, c);
                r2 -= 1;
            }
        }

        /* Fill up on the top */
        for ( ; r2 >= 0; --r2)
        {
            FLD(board, r2, c) = *board->drops[c]++;
            if (board->drops[c] == board->game->drops_end[c])
            {
                board->drops[c] = board->game->drops_begin[c];
            }
        }
    }

    area->r1 = max(0, new_area.r1 - 2);
    area->c1 = max(0, new_area.c1 - 2);
    area->r2 = min(HIG(board), new_area.r2 + 3);
    area->c2 = min(WID(board),  new_area.c2 + 3);
}

/* Finds scoring rows on the board, and removes them, repeating the process
   while scoring rows exist. The total score is returned (or MAX_SCORE, if the
   total score would equal or exceed MAX_SCORE).

   Area is used as the area of interest (which may be only a small part of
   the board that has changed).
*/
static int board_score(Board *board, Rect *area)
{
    int total_score, score, iterations;

    iterations = total_score = 0;
    while ((score = remove_groups(board, area)) > 0)
    {
        fill_columns(board, area);
        total_score += score;
        if (board->score + total_score >= SCORE_LIMIT) break;
        if (++iterations == 10000)
        {
            /* Let's assume we're in an infinite loop */
            total_score = SCORE_LIMIT;
            break;
        }
    }
    board->score += total_score;
    if (board->score > SCORE_LIMIT) board->score = SCORE_LIMIT;

    return total_score;
}

/* Swap fields at given positions */
static void swap_fields(Board *board, int r1, int c1, int r2, int c2)
{
    Field tmp = FLD(board, r1, c1);
    FLD(board, r1, c1) = FLD(board, r2, c2);
    FLD(board, r2, c2) = tmp;
}

static char *file_get_contents(const char *path, size_t *size)
{
    FILE *fp;
    long filesize;
    char *buffer;

    buffer = NULL;
    if ((fp = fopen(path, "rt")) == NULL) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) goto failed;
    filesize = ftell(fp);
    if (filesize < 0) goto failed;
    if (fseek(fp, 0, SEEK_SET) != 0) goto failed;
    if ((buffer = malloc((size_t)filesize)) == NULL) goto failed;
    *size = (size_t)fread(buffer, 1, (size_t)filesize, fp);
    if (*size != (size_t)filesize) goto failed;
    return buffer;

failed:
    fclose(fp);
    free(buffer);
    return NULL;
}

static Board *board_alloc(Game *game)
{
    char *data;
    Board *board;

    data = malloc( sizeof(Board) +
                   game->height*game->width*sizeof(Field) +
                   game->width*sizeof(Field*) );

    if (data == NULL) return NULL;

    board = (Board*)data;
    board->drops  = (Field**)(data + sizeof(Board));
    board->fields = (Field*)(data + sizeof(Board) + game->width*sizeof(Field*));

    return board;
}

static bool read_fields(Game *game, const char *path)
{
    char *buf;
    size_t len, n;
    int r, c;

    if ((buf = file_get_contents(path, &len)) == NULL) return false;

    /* Determine board boundaries */
    game->width = game->height = 0;
    for (n = 0; n < len; ++n)
    {
        if (buf[n] >= '0' && buf[n] <= '1') game->width += 1;
        if (buf[n] == '\n') break;
    }
    for (n = 0; n < len; ++n) if (buf[n] == '\n') game->height += 1;
    if (n > 0 && buf[n-1] != '\n') game->height += 1;

    /* Ensure boundaries are in range. We silently accept larger fields,
       because it is perfectly possible to play the game in only a part of
       the board. */
    if (game->width < 1 || game->height < 1) goto failed;
    if (game->height > MAX_HEIGHT) game->height = MAX_HEIGHT;
    if (game->width  > MAX_WIDTH)  game->width  = MAX_WIDTH;

    /* Allocate state memory */
    game->drops_begin = malloc(game->width*sizeof(Field*));
    game->drops_end   = malloc(game->width*sizeof(Field*));
    if (game->drops_begin == NULL || game->drops_end == NULL) return false;

    /* Allocate initial board */
    game->initial = board_alloc(game);
    game->initial->game = game;
    game->initial->moves = 0;
    game->initial->score = 0;
    game->initial->last_move = NULL;

    /* Initialize fields */
    r = c = 0;
    memset(game->initial->fields, FIELD_BLOCKED, game->width*game->height);
    for (n = 0; n < len; ++n)
    {
        if (buf[n] >= '0' && buf[n] <= '1')
        {
            if (r < game->height && c < game->width && buf[n] == '0')
            {
                FLD(game->initial, r, c) = FIELD_EMPTY;
            }
            c += 1;
        }
        if (buf[n] == '\n')
        {
            c = 0;
            r += 1;
        }
    }

    free(buf);
    return true;

failed:
    free(buf);
    return false;
}

static bool read_columns(Game *game, const char *path)
{
    char *buf;
    int r, c;
    size_t len, n, cnt;
    Field *blocks;

    if ((buf = file_get_contents(path, &len)) == NULL) return false;

    /* Check file format */
    cnt = 0;
    r = c = 0;
    for (n = 0; n < len; ++n)
    {
        if (buf[n] >= '0' && buf[n] <= '9')
        {
            c += 1;
            cnt += 1;
        }
        if (buf[n] == '\n')
        {
            if (c == 0) goto failed; /* empty lines */
            c = 0;
            r += 1;
            if (r == game->width) break;
        }
    }
    if (c > 0) r += 1;

    if (r < game->width) goto failed;  /* not enough lines */

    /* Everything OK. Allocate memory for blocks and drop lists. */
    if ((blocks = malloc(sizeof(Field)*cnt)) == NULL) goto failed;

    /* Assign drops */
    r = 0;
    game->drops_begin[0] = game->drops_end[0] = blocks;
    for (n = 0; n < len; ++n)
    {
        if (buf[n] >= '0' && buf[n] <= '9')
        {
            *game->drops_end[r]++ = FIELD_EMPTY + 1 + buf[n] - '0';
        }
        if (buf[n] == '\n')
        {
            r += 1;
            if (r == game->width) break;
            game->drops_begin[r] = game->drops_end[r] = game->drops_end[r - 1];
        }
    }
    free(buf);

    /* Set drop list pointers in initial board */
    for (c = 0; c < game->width; ++c)
    {
        game->initial->drops[c] = game->drops_begin[c];
    }

    return true;

failed:
    free(buf);
    return false;
}

Game *game_load(const char *dir)
{
    char oldwd[1024];
    Game *game;
    Rect area;

    /* Back up current working dir, then got to new dir */
    if (getcwd(oldwd, sizeof(oldwd)) == NULL || chdir(dir) != 0) return NULL;

    /* Allocate game */
    game = malloc(sizeof(Game));
    if (game == NULL) return NULL;
    memset(game, 0, sizeof(Game));

    /* Read field and column data */
    if (!read_fields(game, "speelveld.txt")) goto failed;
    if (!read_columns(game, "kolommen.txt")) goto failed;

    /* Fill board and set initial score */
    area.r1 = area.c1 = 0;
    area.r2 = game->height;
    area.c2 = game->width;
    fill_columns(game->initial, &area);
    game->initial->score = board_score(game->initial, &area);

    /* Go back to old working dir */
    if (chdir(oldwd) != 0) goto failed;

    return game;

failed:
    /* Clean up allocated resources */
    {
        int e = errno;
        chdir(oldwd);
        game_free(game);
        errno = e;
        return NULL;
    }
}

void game_free(Game *game)
{
    if (game != NULL)
    {
        if (game->drops_begin != NULL)
        {
            free(game->drops_begin[0]);
            free(game->drops_begin);
        }
        if (game->drops_end != NULL)
        {
            free(game->drops_end);
        }
        board_free(game->initial);
        free(game);
    }
}

int board_move(Board *board, int r1, int c1, int r2, int c2, int trace)
{
    int score;
    Rect area;

    area.r1 = max(0, min(r1, r2) - 2);
    area.c1 = max(0, min(c1, c2) - 2);
    area.r2 = min(HIG(board), max(r1, r2) + 3);
    area.c2 = min(WID(board), max(c1, c2) + 3);

    swap_fields(board, r1, c1, r2, c2);
    score = board_score(board, &area);
    if (score > 0)
    {
        ++board->moves;
        if (trace)
        {
            Move *new_move = malloc(sizeof(Move));
            /* FIXME: this should be externally detectable */
            assert(new_move != NULL);
            new_move->prev = board->last_move;
            new_move->ref_count = 1;
            new_move->r1 = r1;
            new_move->c1 = c1;
            new_move->r2 = r2;
            new_move->c2 = c2;
            board->last_move = new_move;
        }
        else
        {
            board->last_move = NULL;
        }
    }
    else
    {
        /* move does not score any rows -- undo it */
        swap_fields(board, r1, c1, r2, c2);
    }

    return score;
}

Board *board_clone(Board *board)
{
    Board *clone;

    clone = board_alloc(board->game);
    if (clone != NULL)
    {
        clone->game = board->game;
        memcpy( clone->fields, board->fields,
            clone->game->width*clone->game->height*sizeof(*clone->fields) );
        memcpy( clone->drops, board->drops,
            clone->game->width*sizeof(*clone->drops) );
        clone->score = board->score;
        clone->moves = board->moves;
        clone->last_move = board->last_move;
        if (clone->last_move != NULL) move_ref(clone->last_move);
    }

    return clone;
}

void board_dump(Board *board, void *fp)
{
    int r, c;

    /* Render board header */
    fprintf(fp, "    ");
    for (r = HIG(board) - 1; r >= 0; --r) fprintf(fp, " %d", r%10);
    fprintf(fp, "  ");
    for (r = 0; r < 20; ++r) fprintf(fp, " %d", r%10);
    fputc('\n', fp);

    fprintf(fp, "    ");
    for (r = HIG(board) - 1; r >= 0; --r) fprintf(fp, "--");
    fprintf(fp, "- ");
    for (r = 0; r < 20; ++r) fprintf(fp, "--");
    fprintf(fp, "-\n");

    /* Render board (on its side) */
    for (c = 0; c < WID(board); ++c)
    {
        printf(" %2d ", c);
        for (r = HIG(board) - 1; r >= 0; --r)
        {
            Field f = FLD(board, r, c);
            char ch = (f == -1) ? '#' :
                      (f ==  0) ? '.' :
                      (f >= 1 && f <= 10) ? (char)('0' + f - 1) : '?';
            fprintf(fp, " %c", ch);
        }
        fprintf(fp, " :");
        for (r = 0; r < 20; ++r)
        {
            int pos = board->drops[c] - board->game->drops_begin[c];
            int len = board->game->drops_end[c] - board->game->drops_begin[c];
            Field f = board->game->drops_begin[c][(pos + r)%len];
            fprintf(fp, " %c", (char)('0' + f - 1));
        }
        fputc('\n', fp);
    }
}

void move_ref(Move *move)
{
    if (move != NULL) ++move->ref_count;
}

void move_deref(Move *move)
{
    Move *prev;

    while (move != NULL)
    {
        assert(move->ref_count > 0);
        if (--move->ref_count > 0) break;
        prev = move->prev;
        free(move);
        move = prev;
    }
}

void board_free(Board *board)
{
    move_deref(board->last_move);
    free(board);
}

/* NOTE: this function uses a lot of stack space! */
void moves_print(Move *move, void *fp)
{
    Move *moves[MOVE_LIMIT];
    size_t size;

    size = 0;
    while (move != NULL)
    {
        moves[size++] = move;
        move = move->prev;
    }

    while (size > 0)
    {
        move = moves[--size];
        fprintf( fp, "%d %d %c\n",
                 (int)move->c1, (int)move->r1,
                 (move->r1 == move->r2) ? 'O' : 'Z' );
    }
}
