
//head for msg.h

#ifndef HOST_MONITOR_H
#define HOST_MONITOR_H

#define HOST_STAT_BUSY 1
#define HOST_STAT_FREE 2
#define HOST_STAT_OFFLINE 3
#define HOST_STAT_ONLINE 4

#define NUM_HOSTS 2
#define MAX_NUM_HOSTS (2*NUM_HOSTS+1)

void host_query();
void host_monitor_transfer_0();
void host_monitor_transfer_1();
void host_monitor_transfer_2();

#endif
