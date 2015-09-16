
//server for easy pbs v0.3
//www.bdyunmu.com
//09/16/2015   	lihui@indiana.edu

//1. memory management
//	if msg send larger than 1MB
//2. threading and task model
//3. note use the memcpy rather than strncpy!
//4. cannot connect to itself.
//5. msg_send (2,1) is wrong while msg_send(1,2) is right, need to check!

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#include "libmsg.h"
#include "libhost.h"
#include "libjobq.h"

int  local_id;
char hostname[64];
int  apt_fd[MAX_NUM_HOSTS];
int  map_fd[MAX_NUM_HOSTS];

pthread_mutex_t work_lock;
pthread_mutex_t next_lock;
pthread_mutex_t stat_lock;

extern pthread_mutex_t hostq_lock;
extern pthread_mutex_t jobq_lock;
extern int job_head;
extern int job_tail;

extern int pipes[2];
extern char hostfile[64];
extern int local_machine_stat;

extern batch_job_info jobq[MAX_JOB_QUEUE];
extern epbs_host_info hostq[(MAX_NUM_HOSTS+1)];

char *ips[(MAX_NUM_HOSTS+1)];
char *hosts[(MAX_NUM_HOSTS+1)];  //server   hosts
		  		 //hosts[0] master nodes
void *client_thread(void *arg)
{

}//void

void epbssrv_configure(){

	sprintf(hostfile,"hosts");
	host_configure("hosts");
	hostq_init();

	fprintf(stderr,"epbssrv_configure.\n");
	
	local_machine_stat = 0;
	int i;
	pthread_mutex_lock(&jobq_lock);
	batch_job_info *job;
	for(i=0;i<MAX_JOB_QUEUE;i++){
		job = &(jobq[i]);
		job->job_id = -1;
		job->job_stat = JOB_STAT_UNKNOW;
		job->username[0] = '\0';
		job->pcomm[0] = '\0';
	}//for
	pthread_mutex_unlock(&jobq_lock);
		
}//void

//[type][src][dist][payload_len]
//   0  null    message
//   1	write data 
//   2  control message
//   3  read  data 

void deal_msg_request(){
	
	int srv_fd;
        int srv_listen_fd;
        if((srv_listen_fd=socket(AF_INET,SOCK_STREAM,0))==-1)
        {
        fprintf(stderr,"socket error:%s\n\a",strerror(errno));
        exit(1);
        }//if
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        int portnumber = MSG_PORT_NUMBER;
	
	fprintf(stderr,"deal msg request portnumber:%d\n",MSG_PORT_NUMBER);

        bzero(&server_addr,sizeof(struct sockaddr_in));

        server_addr.sin_family=AF_INET;
        server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        server_addr.sin_port=htons(portnumber);

        if(bind(srv_listen_fd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
        {
        fprintf(stderr,"deal msg request bind error:%s\n\a",strerror(errno));
        exit(1);
        }//if

        if(listen(srv_listen_fd,5)==-1)
        {
        fprintf(stderr,"listen error:%s\n\a",strerror(errno));
        exit(1);
        }//if

	while(1){

	int sin_size=sizeof(struct sockaddr_in);
        if((srv_fd=accept(srv_listen_fd,(struct sockaddr *)(&client_addr),&sin_size))==-1)
        {
        fprintf(stderr,"accept error:%s\n\a",strerror(errno));
        exit(1);
        }//if
        else
        {
        fprintf(stderr,"srv accept connection\n");
        }//else

        char msgbuffer[MSG_BUFF_SIZE];
        int nbytes = read(srv_fd,(char *)msgbuffer,MSG_BUFF_SIZE);
       	int len = *((int *)msgbuffer+3);
	int rbytes = len + sizeof(int)*MSG_HEAD_SIZE;
	fprintf(stderr,"srv done get rbytes:%d len:%d read:%d bytes\n",
					rbytes,len,nbytes);
	int wbytes = write(pipes[1],msgbuffer,nbytes);

	}//while

	close(srv_fd);
	close(srv_listen_fd);

}


void *host_status_thread(void *argv){
	host_status();
}

void *cancel_job_thread(void *argv){
	cancel_job_request();
}//void

void *host_query_thread(void *argv){
	host_query();
}//void

void *interactive_job_thread(void *argv){
	deal_interactive_job_request();
}//void

void *job_queue_push_thread(void *argv){
	job_queue_push();
}//void

void *job_queue_pop_thread(void *argv){
	job_queue_pop();
}//void

void *job_status_query_thread(void *argv){
	job_status_query();
}//void

void *job_queue_result_thread(void *argv){
	job_queue_result();
}//void

void *msg_process(void *argv){
	deal_msg_request();
}//void

void *job_monitor_thread(void *argv){
	job_monitor();
}//void

int main(int argc, char **argv){
	
	pthread_t pid_msg;
	pthread_t pid_job;
	pthread_t pid_host;
	
	pthread_t pid_host_query;
	pthread_t pid_host_result;
	pthread_t pid_host_stat;
	
	pthread_t pid_job_push_queue;
	pthread_t pid_job_pop_queue;
	pthread_t pid_job_query;
	pthread_t pid_job_result;
	pthread_t pid_job_monitor;

	epbssrv_configure();
	pthread_mutex_init(&work_lock,NULL);
	pthread_mutex_init(&next_lock,NULL);
	pthread_mutex_init(&stat_lock,NULL);
	pthread_mutex_init(&jobq_lock,NULL);
	job_head = 0;
	job_tail = 0;
	pipe(pipes);
	
	//pthread_mutex_lock(&work_lock);
	//pthread_mutex_lock(&next_lock);

	//host_configure file 
	//serer_configure file

	pthread_create(&pid_msg,NULL,msg_process,NULL);
	pthread_create(&pid_job,NULL,interactive_job_thread,NULL);
	pthread_create(&pid_host,NULL,cancel_job_thread,NULL);

	if(local_id == 1){
	pthread_create(&pid_job_monitor,NULL,job_monitor_thread,NULL);
	pthread_create(&pid_job_push_queue, NULL, job_queue_push_thread, NULL);
	pthread_create(&pid_job_pop_queue, NULL, job_queue_pop_thread, NULL);
	pthread_create(&pid_job_query, NULL, job_status_query_thread,NULL);
	pthread_create(&pid_job_result,NULL, job_queue_result_thread,NULL);
	pthread_create(&pid_host_query, NULL, host_query_thread,NULL);
	pthread_create(&pid_host_stat, NULL, host_status_thread,NULL);
	}//if

	pthread_join(pid_job,NULL);
	pthread_join(pid_msg,NULL);
	pthread_join(pid_host,NULL);

	if(local_id == 1){
	pthread_join(pid_job_monitor,NULL);
	pthread_join(pid_job_pop_queue,NULL);
	pthread_join(pid_job_push_queue,NULL);
	pthread_join(pid_job_query,NULL);
	pthread_join(pid_job_result,NULL);
	pthread_join(pid_host_query,NULL);
	pthread_join(pid_host_stat,NULL);
	}//if

}//int main

