#ifndef TasksTracker_H
#define TasksTracker_H

void setup_TrackerTasks();
void tasksendStatusMessageTX(void * parameter);
void tasksendRegularBeaconTX(void * parameter);
void tasksendSmartBeaconTX(void * parameter);
void tasksendDeepSleepBeaconTX(void * parameter);


#endif