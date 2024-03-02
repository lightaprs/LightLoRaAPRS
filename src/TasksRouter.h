#ifndef TasksRouter_H
#define TasksRouter_H

void setup_RouterTasks();
void tasksendRXPacketsToQueueR(void * parameter);
void tasksendRXPacketsToRF(void * parameter);
void tasksendRouterLocationToRF(void * parameter);


#endif