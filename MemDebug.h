#ifndef MEM_DEBUG_H_INCLUDED
#define MEM_DEBUG_H_INCLUDED

/* Simple wrappers for C-style allocation functions (malloc, calloc, realloc
   and free) which can be used to detect memory leaks and freeing of invalid
   or pointers.

   Not designed for high performance.
   IMPORTANT: these functions ARE NOT THREAD SAFE!
              Use only in singlethreaded applications.
*/

#include <stdio.h>
#include <stdlib.h>

/* Standard C memory allocation replacements */
void *mem_debug_malloc(const char *file, int line, size_t size);
void *mem_debug_realloc(const char *file, int line, void *ptr, size_t size);
void *mem_debug_calloc(const char *file, int line, size_t cnt, size_t size);
void mem_debug_free(const char *file, int line, void *ptr);

/* Write a memory leak report to the given file pointer. */
void mem_debug_report(FILE *fp, int enabled);

/* Report memory leaks to standard error at exit. */
void mem_debug_report_at_exit(FILE *fp, int enabled);

/* The macro's below are used to select whether the C library memory
   allocation functions are called directly, or the debugging wrappers
   defined here are used instead. */
#ifdef MEM_DEBUG

#define malloc(size)        mem_debug_malloc(__FILE__, __LINE__, size)
#define realloc(ptr, size)  mem_debug_realloc(__FILE__, __LINE__, ptr, size)
#define calloc(cnt, size)   mem_debug_calloc(__FILE__, __LINE__, cnt, size)
#define free(ptr)           mem_debug_free(__FILE__, __LINE__, ptr)

#define mem_debug_report(fp)            mem_debug_report(fp, 1)
#define mem_debug_report_at_exit(fp)    mem_debug_report_at_exit(fp, 1)

#else

#define mem_debug_report(fp)            mem_debug_report(fp, 0)
#define mem_debug_report_at_exit(fp)    mem_debug_report_at_exit(fp, 0)

#endif

#endif /* ndef MEM_DEBUG_H_INCLUDED */
