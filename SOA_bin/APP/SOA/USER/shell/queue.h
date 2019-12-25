#ifndef __QUEUE_H
#define __QUEUE_H

#define QUEUE_SIZE                                              1024

typedef struct
{
	char data[QUEUE_SIZE];
	int rear,front;
}queue_t;

void queue_init(queue_t *q);
int queue_push(queue_t* q, char data);
int queue_pop(queue_t* q, char* data);

#endif