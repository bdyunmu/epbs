
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

void *job_srv_process(void *);

void client_submit_single_job(int local_id, int remote_id, char *commstr, int num_req_nodes);
void client_submit_job_done();
void client_cancel_job(int );

int client_query_job_status(int job_id);
int *client_query_all_job_stat();
int client_query_host_status(int machine_id);
int single_job_submit(int local_id, int remote_id, char *command, int num_req_nodes);
void server_single_job_submit(int local_id, int remote_id, char *command, int job_id);
void server_single_job_cancel(int job_id_, int host_id_);
void cancel_job_request();
void job_monitor();//depricated
void job_monitor_utransfer();//depricated 9/30/2015
void job_monitor_etransfer();//new 10/7/2015
void job_queue_result_0();
void job_queue_result_1();
void job_status_query();
void job_queue_pop();
void job_queue_push();
void deal_interactive_job_request();
#endif
