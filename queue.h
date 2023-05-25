#ifndef _QUEUE_H
#define _QUEUE_H

typedef struct queue_node queue_node;

typedef struct {
	queue_node *head, *tail;
	unsigned size;
} queue;

struct queue_node {
	void* data;
	queue_node* next;
};

/**
 * Creates a returns a pointer to a new queue.
 * 
 * @return a pointer to a new queue
*/
queue* queue_create();

/**
 * Adds the specified element to the end of the specified queue.
 * 
 * @param q the queue to add the element to
 * @param data the element to add to the queue
*/
void queue_enqueue(queue* q, void* data);

/**
 * Removes and returns the element at the front of the specified queue.
 * 
 * @param q the queue to remove the element from
 * @return the element at the front of the specified queue
*/
void* queue_dequeue(queue* q);

#endif