#include "queue.h"
#include "wrapper_functions.h"

/* Helper Functions */

static queue_node* queue_node_create(void *data) {
	queue_node* node = Calloc(1, sizeof(queue_node));
	node->data = data;
	return node;
}

/* Header Implementation */

queue* queue_create() {
	return Calloc(1, sizeof(queue));
}

void queue_enqueue(queue *q, void *data) {
	queue_node* node = queue_node_create(data);

	if(q->size == 0)
		q->head = node;
	else
		q->tail->next = node;

	q->tail = node;
	q->size++;
}

void* queue_dequeue(queue *q) {
	queue_node* node = q->head;
	void* data = node->data;

	q->head = node->next;
	q->size--;

	Free(node);
	return data;
}