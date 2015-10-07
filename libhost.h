
//head for msg.h

#ifndef HOST_H
#define HOST_H

#define HOST_STAT_BUSY 1
#define HOST_STAT_FREE 2
#define HOST_STAT_OFFLINE 3
#define HOST_STAT_ONLINE 4

#define NUM_HOSTS 2
#define MAX_NUM_HOSTS (2*NUM_HOSTS+1)
void *host_srv_process(void *);

void hostq_init();
void epbs_host_status();
extern "C"{
void read_host_config_file(char *hostfile);
}
//void host_query();
char *get_host_ip(int dist_id);

typedef struct epbs_host_infor_{
	int host_id;
	int host_stat;
	int num_cores;
	int num_mem;
	clock_t time1;
	clock_t time2;
	char ip[32];
} epbs_host_info;

extern int num_hosts;
extern int local_id;
extern int pcstat[(MAX_NUM_HOSTS)];
#endif
