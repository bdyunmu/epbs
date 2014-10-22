
//head for msg.h

#ifndef JOB_H
#define JOB_H

#define MAX_JOB_QUEUE 32

#define JOB_STAT_FINISHED 3
#define JOB_STAT_RUNNING 2
#define JOB_STAT_INQUEUE 1
#define JOB_STAT_UNKNOW 4

typedef struct batch_job_info_	{
	char pcomm[256];
	char username[32];
	int  num_req_nodes;
	time_t  time0;//reserved time in seconds
	time_t  time1;//time to start the job
	time_t  time2;//time to finish the job
	time_t  time3;//time 
	int  	job_stat;
	int  	job_id;
	pthread_t  tid;
} batch_job_info;

int client_query_job_stat(int job_id);
int *client_query_all_job_stat();
int client_query_host_stat(int machine_id);
int single_job_submit(int local_id, int remote_id, char *command, int num_req_nodes);

#endif
