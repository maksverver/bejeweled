#include <stdio.h>
#include <stdlib.h>

struct Ptr
{
    struct Ptr *next;   /* pointer to next node in linked list */

    void *ptr;          /* registered pointer */
    size_t size;        /* allocated size */
    const char *file;   /* file where the allocation was made */
    int line;           /* line where the allocation was made */
};

/* Global variables */

/* Hash table containing linked lists of registered pointers */
#define HT_SIZE (1234567)
static struct Ptr *ptrs[HT_SIZE];

/* For running the at-exit handler: */
static int at_exit_registered;
static FILE *at_exit_fp;



void add_ptr(void *ptr, size_t size, const char *file, int line)
{
    struct Ptr *p = malloc(sizeof(struct Ptr));

    if (p != NULL)
    {
        size_t i = ((size_t)ptr)%HT_SIZE;
        p->next = ptrs[i];
        p->ptr  = ptr;
        p->size = size;
        p->file = file;
        p->line = line;
        ptrs[i] = p;
    }
    else
    {
        fprintf(stderr, "add_ptr() allocation failed\n");
        abort();
    }
}

int del_ptr(void *ptr)
{
    struct Ptr **pp;
    size_t i = ((size_t)ptr)%HT_SIZE;

    for (pp = &ptrs[i]; *pp; pp = &(*pp)->next)
    {
        if ((*pp)->ptr == ptr)
        {
            struct Ptr *p = *pp;
            *pp = (*pp)->next;
            free(p);
            return 1;
        }
    }

    return 0;
}

void *mem_debug_malloc(const char *file, int line, size_t size)
{
    void *data = malloc(size);

    if (data == NULL && size != 0)
    {
        fprintf(stderr, "%s:%d  malloc(%zd) failed\n", file, line, size);
    }

    if (data != NULL) add_ptr(data, size, file, line);

    return data;
}

void *mem_debug_realloc(const char *file, int line, void *ptr, size_t size)
{
    /* FIXME: if "ptr" is invalid, then the call to realloc is invalid and
              may crash the application before we have a chance to verify
              the pointer in del_ptr(); however, we shouldn't del_ptr first,
              because the pointer remains valid if realloc() failed. */

    void *data = realloc(ptr, size);

    if (data == NULL && size != 0)
    {
        fprintf( stderr, "%s:%d  realloc(%p, %zd) failed\n",
                         file, line, ptr, size );
    }
    else
    if (ptr != NULL && !del_ptr(ptr))
    {
        fprintf( stderr, "[%s:%d] invalid pointer passed to realloc(%p, %zd)\n",
                         file, line, ptr, size );
    }

    if (data != NULL) add_ptr(data, size, file, line);

    return data;
}

void *mem_debug_calloc(const char *file, int line, size_t cnt, size_t size)
{
    void *data = calloc(cnt, size);

    if (data == NULL && size != 0 && cnt != 0)
    {
        fprintf( stderr, "[%s:%d] calloc(%zd, %zd) failed\n",
                         file, line, cnt, size );
    }

    if (data != NULL) add_ptr(data, cnt*size, file, line);

    return data;
}

void mem_debug_free(const char *file, int line, void *ptr)
{
    if (ptr == NULL) return;

    if (!del_ptr(ptr))
    {
        fprintf( stderr, "[%s:%d] invalid pointer passed to free(%p)\n",
                         file, line, ptr );
    }

    free(ptr);
}

void mem_debug_report(FILE *fp, int enabled)
{
    if (!enabled)
    {
        fprintf(fp, "Memory leak detection not enabled!\n");
        return;
    }

    struct Ptr *p;
    size_t i, cnt = 0;;
    for (i = 0; i < HT_SIZE; ++i)
    {
        for (p = ptrs[i]; p != NULL; p = p->next)
        {
            fprintf(fp, "Pointer %p with size %zd leaked, "
                        "allocated in file %s at line %d.\n",
                        p->ptr, p->size, p->file, p->line );
            ++cnt;
        }
    }

    if (cnt == 0) fprintf(fp, "No memory leaks present!\n");
}

static void report_at_exit_enabled()
{
    mem_debug_report(at_exit_fp, 1);
}

static void report_at_exit_disabled()
{
    mem_debug_report(at_exit_fp, 0);
}

void mem_debug_report_at_exit(FILE *fp, int enabled)
{
    if (at_exit_registered) return;

    at_exit_registered = 1;
    at_exit_fp = fp;

    atexit(enabled ? &report_at_exit_enabled : &report_at_exit_disabled);
}
