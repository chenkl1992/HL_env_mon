#include "queue.h"

void queue_init(queue_t *q)
{
    q->front = 0;
    q->rear = 0;
}

int queue_push(queue_t* q, char data)
{
	if((q->rear + 1 ) % QUEUE_SIZE == q->front)
	{
		return -1;
	}
		
	else
	{
		q->rear = (q->rear +1 )% QUEUE_SIZE;
		q->data[q->rear] = data;	
	}
	return 0;
}

int queue_pop(queue_t* q, char* data)
{
	char x;
    
	if(q->rear == q->front)
	{
		return -1;
	}	
	else
	{
		q->front = (q->front +1 )% QUEUE_SIZE;
		x = q->data[q->front];	
		*data = x;
	}
    
	return 0;
}