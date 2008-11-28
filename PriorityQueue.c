#include <assert.h>
#include "PriorityQueue.h"
#include "MemDebug.h"

/*
  Note:

  push/pop operations can be optimized some more, since all elements are
  individually swapped up/down, while it is more efficient to first swap the
  elements on the path and then swap the new element into its final place.

  Secondly, it may be worthwhile to force inlining of heap_pop and heap_push
  functions, so that the run-time checks to see whether the heap passed is
  pq->min_heap or pq->max_heap can be optimized away.
*/

#if 0
/* Used for debugging */
static void heap_check(PriorityQueue *pq, HeapNode *heap)
{
    size_t cur;
    for (cur = 0; cur < pq->size; ++cur)
    {
        if (cur > 0)
        {
            int d = pq->compare(heap[(cur-1)/2].data, heap[cur].data);
            if ((heap == pq->min_heap ? +d : -d) > 0)
            {
                printf("heap violation at position %zd\n", cur);
                abort();
            }
        }
        if (heap[cur].other->other != &heap[cur])
        {
            printf("cross-reference violation at position %zd\n", cur);
            abort();
        }
    }
}
#else
#define heap_check(pq, heap)
#endif

/* Swap to heap elements */
static void heap_swap(HeapNode *pq, size_t i, size_t j)
{
    HeapNode tmp;

    tmp = pq[i];
    pq[i] = pq[j];
    pq[j] = tmp;

    pq[i].other->other = &pq[i];
    pq[j].other->other = &pq[j];
}

static void heap_push(PriorityQueue *pq, HeapNode *heap, HeapNode *elem)
{
    size_t cur = elem - heap;
    while (cur > 0)
    {
        size_t par = (cur - 1)/2;

        /* Stop if parent is not smaller than current node */
        int d = pq->compare(heap[par].data, heap[cur].data);
        if ((heap == pq->min_heap ? +d : -d) <= 0) break;

        /* Swap with parent */
        heap_swap(heap, cur, par);
        cur = par;
    }
}

static void heap_pop(PriorityQueue *pq, HeapNode *heap, HeapNode *node)
{
    /* Swap selected note to the end */
    size_t cur  = node - heap;
    size_t size = pq->size - 1;
    heap_swap(heap, cur, size);

    /* See if the new element must move up or down */
    {
        int d = pq->compare(heap[cur].data, heap[size].data);
        if ((heap == pq->min_heap ? +d : -d) <= 0)
        {
            /* new element is smaller than old element, so move it up */
            heap_push(pq, heap, &heap[cur]);
            return;
        }
    }

    /* Restore heap property by moving down */
    size_t child;
    while ((child = 2*cur + 1) < size)
    {
        /* Select child to compare against (smallest of the two) */
        if (child + 1 < size)
        {
            int d = pq->compare(heap[child].data, heap[child + 1].data);
            if ((heap == pq->min_heap ? +d : -d) > 0) child = child + 1;
        }

        /* Stop if current element is not greater than child */
        int d = pq->compare(heap[cur].data, heap[child].data);
        if ((heap == pq->min_heap ? +d : -d) <= 0) break;

        /* Swap current node with smallest child */
        heap_swap(heap, cur, child);
        cur = child;
    }
}

PriorityQueue *pq_create(size_t capacity, pq_compare_t *compare)
{
    HeapNode *min_heap = NULL, *max_heap = NULL;
    PriorityQueue *pq = NULL;

    /* Allocate required memory */
    min_heap = malloc(capacity*sizeof(HeapNode));
    if (min_heap == NULL) goto failed;
    max_heap = malloc(capacity*sizeof(HeapNode));
    if (max_heap == NULL) goto failed;
    pq = malloc(sizeof(PriorityQueue));
    if (pq == NULL) goto failed;

    /* Initialize data structure */
    pq->size        = 0;
    pq->capacity    = capacity;
    pq->compare     = compare;
    pq->min_heap    = min_heap;
    pq->max_heap    = max_heap;

    return pq;

failed:
    free(min_heap);
    free(max_heap);
    free(pq);
    return NULL;
}

void pq_destroy(PriorityQueue *pq)
{
    if (pq == NULL) return;
    free(pq->min_heap);
    free(pq->max_heap);
    free(pq);
}

void *pq_pop_min(PriorityQueue *pq)
{
    assert(pq->size > 0);

    void *res = pq_get_min(pq);

    HeapNode *min_node = &pq->min_heap[0];
    HeapNode *max_node = min_node->other;

    heap_pop(pq, pq->min_heap, min_node);
    heap_pop(pq, pq->max_heap, max_node);

    --pq->size;

    heap_check(pq, pq->min_heap);
    heap_check(pq, pq->max_heap);

    return res;
}

void *pq_pop_max(PriorityQueue *pq)
{
    assert(pq->size > 0);

    void *res = pq_get_max(pq);

    HeapNode *max_node = &pq->max_heap[0];
    HeapNode *min_node = max_node->other;

    heap_pop(pq, pq->min_heap, min_node);
    heap_pop(pq, pq->max_heap, max_node);

    --pq->size;

    heap_check(pq, pq->min_heap);
    heap_check(pq, pq->max_heap);

    return res;
}

void pq_push(PriorityQueue *pq, void *elem)
{
    assert(pq->size < pq->capacity);

    /* Add element at end of heaps */
    pq->min_heap[pq->size].data  = elem;
    pq->min_heap[pq->size].other = &pq->max_heap[pq->size];
    pq->max_heap[pq->size].data  = elem;
    pq->max_heap[pq->size].other = &pq->min_heap[pq->size];

    /* Restore heaps */
    heap_push(pq, pq->min_heap, &pq->min_heap[pq->size]);
    heap_push(pq, pq->max_heap, &pq->max_heap[pq->size]);

    ++pq->size;

    heap_check(pq, pq->min_heap);
    heap_check(pq, pq->max_heap);
}
