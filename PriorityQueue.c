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
static void heap_check(HeapNode *heap, size_t size)
{
    size_t cur;
    for (cur = 0; cur < size; ++cur)
    {
        if (cur > 0 && heap[(cur - 1)/2].prio > heap[cur].prio)
        {
            printf("heap violation at position %zd\n", cur);
            abort();
        }
        if ( heap[cur].other->other != &heap[cur] )
        {
            printf("cross-reference violation at position %zd\n", cur);
            abort();
        }
        if ( -heap[cur].prio != heap[cur].other->prio )
        {
            printf("inconsistent priorities at position %zd\n", cur);
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

/* Push the selected element into a min-heap.
   the subtree rooted at elem is assumed to be a valid min-heap already. */
static void heap_push(HeapNode *heap, HeapNode *elem)
{
    size_t cur = elem - heap;
    while (cur > 0)
    {
        size_t par = (cur - 1)/2;

        /* Stop if parent is not larger than current node */
        if (heap[par].prio <= heap[cur].prio) break;

        /* Swap with parent */
        heap_swap(heap, cur, par);
        cur = par;
    }
}

/* Pop the selected element from a min-heap. */
static void heap_pop(HeapNode *heap, size_t size, HeapNode *node)
{
    /* Swap selected note to the end */
    size_t cur  = node - heap;
    heap_swap(heap, cur, --size);

    /* See if the new element must move up or down */
    if (heap[cur].prio <= heap[size].prio)
    {
        /* new element is smaller than old element, so move it up */
        return heap_push(heap, &heap[cur]);
    }

    /* Restore heap property by moving down */
    size_t child;
    while ((child = 2*cur + 1) < size)
    {
        /* Select child to compare against (smallest of the two) */
        if (child + 1 < size && heap[child + 1].prio < heap[child].prio)
        {
            child = child + 1;
        }

        /* Stop if current element is not greater than child */
        if (heap[cur].prio <= heap[child].prio) break;

        /* Swap current node with smallest child */
        heap_swap(heap, cur, child);
        cur = child;
    }
}

PriorityQueue *pq_create(size_t capacity)
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

    void *res = pq_min_data(pq);

    HeapNode *min_node = &pq->min_heap[0];
    HeapNode *max_node = min_node->other;

    heap_pop(pq->min_heap, pq->size, min_node);
    heap_pop(pq->max_heap, pq->size, max_node);

    --pq->size;

    heap_check(pq->min_heap, pq->size);
    heap_check(pq->max_heap, pq->size);

    return res;
}

void *pq_pop_max(PriorityQueue *pq)
{
    assert(pq->size > 0);

    void *res = pq_max_data(pq);

    HeapNode *max_node = &pq->max_heap[0];
    HeapNode *min_node = max_node->other;

    heap_pop(pq->min_heap, pq->size, min_node);
    heap_pop(pq->max_heap, pq->size, max_node);

    --pq->size;

    heap_check(pq->min_heap, pq->size);
    heap_check(pq->max_heap, pq->size);

    return res;
}

void pq_push(PriorityQueue *pq, int prio, void *data)
{
    assert(pq->size < pq->capacity);
    assert(prio >= 0 || -prio > 0);   /* check for positive integer overflow */

    /* Add element at end of heaps */
    pq->min_heap[pq->size].data  = data;
    pq->min_heap[pq->size].prio  = +prio;
    pq->min_heap[pq->size].other = &pq->max_heap[pq->size];
    pq->max_heap[pq->size].data  = data;
    pq->max_heap[pq->size].prio  = -prio;
    pq->max_heap[pq->size].other = &pq->min_heap[pq->size];

    /* Restore heaps */
    heap_push(pq->min_heap, &pq->min_heap[pq->size]);
    heap_push(pq->max_heap, &pq->max_heap[pq->size]);

    ++pq->size;

    heap_check(pq->min_heap, pq->size);
    heap_check(pq->max_heap, pq->size);
}
