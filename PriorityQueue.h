#ifndef PRIORITY_QUEUE_H_INCLUDED
#define PRIORITY_QUEUE_H_INCLUDED

/* A simple min-max priority queue that stores pointers only.
   Implemented using two separate heaps with cross references.
*/

#include <stdlib.h>

/* Priority queue node structure; should not be accessed directly. */
typedef struct HeapNode
{
    int             prio;       /* priority */
    void            *data;      /* stored pointer */
    struct HeapNode *other;     /* reference to element in other queue */
} HeapNode;

/* Priority queue implementation structure; should not be accessed directly. */
typedef struct PriorityQueue
{
    size_t          size, capacity;
    struct HeapNode *min_heap, *max_heap;
} PriorityQueue;


/* Create a priority queue data structure with the given capacity
   Returns NULL if memory allocation failes. */
PriorityQueue *pq_create(size_t capacity);

/* Destroy a priority queue, freeing all allocated resources. */
void pq_destroy(PriorityQueue *pq);

/* Return the capacity (maximum size) of the priority queue. */
#define pq_capacity(pq) ((pq)->capacity)

/* Return the size of the priority queue. */
#define pq_size(pq) ((pq)->size)

/* Return whether the priority queue is empty. */
#define pq_empty(pq) (pq_size(pq) == 0)

/* Return whether the priority queue is full. */
#define pq_full(pq) (pq_size(pq) == pq_capacity(pq))

/* Return the minimum/maximum element in the queue */
#define pq_min_data(pq) ((pq)->min_heap[0].data)
#define pq_max_data(pq) ((pq)->max_heap[0].data)

/* Return the priority of the minimum/maximum element in the queue */
#define pq_min_prio(pq) ((pq)->min_heap[0].prio)
#define pq_max_prio(pq) ((pq)->max_heap[0].prio)

/* Remove the minimum/maximum element from the queue and return it */
void *pq_pop_min(PriorityQueue *pq);
void *pq_pop_max(PriorityQueue *pq);

/* Add an element to the queue */
void pq_push(PriorityQueue *pq, int prio, void *data);

#endif /*ndef PRIORITY_QUEUE_H_INCLUDED */
