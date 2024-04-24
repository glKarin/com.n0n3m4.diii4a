
#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <stddef.h>
#include "qtypes.h"

typedef struct taskqueue_task_s
{
	// if not NULL, this task must be done before this one will dequeue (faster than simply calling TaskQueue_Yield immediately)
	struct taskqueue_task_s *preceding;

	// use TaskQueue_IsDone() to poll done status
	volatile int done;

	// function to call, and parameters for it to use
	void(*func)(struct taskqueue_task_s *task);
	// general purpose parameters
	void *p[2];
	size_t i[2];

	unsigned int yieldcount; // number of times this task has been requeued - each task counts only once for purposes of tasksperthread averaging
}
taskqueue_task_t;

// queue the tasks to be executed, but does not start them (until TaskQueue_WaitforTaskDone is called)
void TaskQueue_Enqueue(int numtasks, taskqueue_task_t *tasks);

// if the task can not be completed due yet to preconditions, just enqueue it again...
void TaskQueue_Yield(taskqueue_task_t *t);

// polls for status of task and returns the result, does not cause tasks to be executed (see TaskQueue_WaitForTaskDone for that)
qbool TaskQueue_IsDone(taskqueue_task_t *t);

// triggers execution of queued tasks, and waits for the specified task to be done
void TaskQueue_WaitForTaskDone(taskqueue_task_t *t);

// convenience function for setting up a task structure.  Does not do the Enqueue, just fills in the struct.
void TaskQueue_Setup(taskqueue_task_t *t, taskqueue_task_t *preceding, void(*func)(taskqueue_task_t *), size_t i0, size_t i1, void *p0, void *p1);

// general purpose tasks
// t->i[0] = number of tasks in array
// t->p[0] = array of taskqueue_task_t to check
void TaskQueue_Task_CheckTasksDone(taskqueue_task_t *t);

void TaskQueue_Init(void);
void TaskQueue_Shutdown(void);
void TaskQueue_Frame(qbool shutdown);

#endif
