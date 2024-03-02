#ifndef TasksGateway_H
#define TasksGateway_H

void setup_GatewayTasks();
void tasksendRXPacketsToQueueGW(void * parameter);
void tasksendRXPacketsToAPRSIS(void * parameter);
void tasksendiGateLocationToAPRSIS(void * parameter);
void taskcheckAPRSISConnection(void * parameter);
void taskAPRSISGetMessages(void * parameter);

#endif