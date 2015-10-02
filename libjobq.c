
//host component for easy pbs v0.2
//www.bdyunmu.com
//9/16/2015	lihui@indiana.edu

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

#include <signal.h>
#include <iostream>

#include "libmsg.h"
#include "libhost.h"
#include "libjobq.h"

#include "PPTimers.h"
#include "utransfer.h"

batch_job_info jobq[MAX_JOB_QUEUE];

int jobnodes[MAX_JOB_QUEUE];
int nodesstat[MAX_NUM_HOSTS];

int job_head;
int job_tail;

pthread_t job_thread_id;
pthread_mutex_t jobq_lock;
extern char *hosts[MAX_JOB_QUEUE];
extern int pcstat[MAX_NUM_HOSTS];

void * execl_thread(void *argv){

        fprintf(stderr,"execl_thread comm:%s\n",argv);
        char *pcomm = (char *)argv;
        if(strcmp(pcomm,"sleep") == 0){
                        msg_init();
			sleep(100);
                        msg_exit();
        }//if
	char emptyargv[10];
	sprintf(emptyargv,"");
	pid_t pid = fork();
	if(pid == 0){
	execl(pcomm,emptyargv);	
	}else{
	printf("execl_thread is done.\n");
	}
}       //void

void job_configure(){
	fprintf(stderr,"job_configure\n");	
}//void

void deal_interactive_job_request(){
	
        int i;
        int portnumber;
	// = UDP_INTER_JOB_SRV_PORT_NUMBER;
	fprintf(stderr,"deal job request portnumber:%d\n",UDP_INTER_JOB_SRV_PORT_NUMBER);

        static int count = 0;
        int host_stat;

	while(1){

        portnumber = UDP_INTER_JOB_SRV_PORT_NUMBER;
        char recv_buffer[MSG_BUFF_SIZE];
        int nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);
	
	if(nbytes <=0){
	sleep(1);
	continue;
	}//if

	int type = *((int *)recv_buffer);
	
	if(type == (MSG_TYPE_JOB_SUBMIT)){

		int tmp;
                int src  = *((int *)recv_buffer+1);
                int dist = *((int *)recv_buffer+2);
                int job_id = *((int *)recv_buffer+3);
                int pcomm_len = *((int *)recv_buffer+4);
                char *pcomm = (char *)((int *)recv_buffer+MSG_HEAD_SIZE);
                pcomm[pcomm_len] = '\0';

		fprintf(stderr,"deal job request read %d bytes get pcommand:%s job_id:%d\n",
                nbytes,pcomm,job_id);

                host_stat = HOST_STAT_BUSY;

                int result;
                pthread_t pt;
                result = pthread_create(&pt,NULL,execl_thread,pcomm);
		sleep(1);
		job_thread_id = pt;

                if(result!=0){
		fprintf(stderr,"deal job request pthread_create error %s\n", strerror(errno));
                fprintf(stderr,"deal job request pthread_cancling...\n");
                pthread_cancel(pt);
                exit(1);
                }//else
                else{
        	fprintf(stderr,"deal job request created new thread\n");
                }//else

                pthread_join(pt,NULL);
                host_stat = HOST_STAT_FREE;

                int server_machine_id = 1;
                char *send_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
                *((int *)send_buffer) = MSG_TYPE_JOB_RETURN;
                *((int *)send_buffer+1) = local_id;
                *((int *)send_buffer+2) = server_machine_id;
                *((int *)send_buffer+3) = job_id;
                *((int *)send_buffer+4) = JOB_STAT_FINISHED;

	portnumber = UDP_JOB_RELT_SRV_PORT_NUMBER;
        udp_send(local_id,server_machine_id,send_buffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);

                int host_id = local_id;
                *((int *)send_buffer) = MSG_TYPE_HOST;
                *((int *)send_buffer+1) = local_id;
                *((int *)send_buffer+2) = server_machine_id;
                *((int *)send_buffer+3) = host_id;
                *((int *)send_buffer+4) = host_stat;
                portnumber = UDP_HOST_STAT_SRV_PORT_NUMBER;
        udp_send(local_id,server_machine_id,send_buffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);
	
	}//
	
        }//while

}

void cancel_job_request(){

	fprintf(stderr,"cancel job request portnumber:%d\n",
                                UDP_CANCEL_JOB_SRV_PORT_NUMBER);

        int i;
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        int sin_size = 0;
        int portnumber = UDP_CANCEL_JOB_SRV_PORT_NUMBER;

        int host_fd;
        if((host_fd=socket(AF_INET,SOCK_DGRAM,0))==-1)
        {
        fprintf(stderr,"socket error:%s\n\a",strerror(errno));
        exit(1);
        }//if

        static int count = 0;
        char recv_buffer[MSG_BUFF_SIZE];
        //char send_buffer[MSG_HEAD_SIZE*sizeof(int)];

	while(1){

        sin_size=sizeof(struct sockaddr_in);
	fprintf(stderr,"debug udp cancel_job srv request before while!\n");
        int nbytes = udp_recv(local_id,0,recv_buffer,
                        MSG_BUFF_SIZE,UDP_CANCEL_JOB_SRV_PORT_NUMBER);

	fprintf(stderr,"debug udp cancel_job srv request\n");
        
        if(nbytes>0){
                int type = *((int *)recv_buffer);
                int src  = *((int *)recv_buffer+1);
                int dist = *((int *)recv_buffer+2);
                int job_id = *((int *)recv_buffer+3);

	fprintf(stderr,"debug cancel job before pthread_cancel\n");
	int res = pthread_cancel(job_thread_id);

	//if (res == PTHREAD_CANCELED)
        //      fprintf(stderr, "debug cancel_job job:%d was canceled\n",job_id);

	fprintf(stderr,"cancel job request done for job_id:%d res:%d\n",
						job_id, res);

#if 0
                //query number of free nodes
                *((int *)send_buffer ) = MSG_TYPE_JOB_CANCEL;
                *((int *)send_buffer +1) = dist;
                *((int *)send_buffer +2) = src;
                *((int *)send_buffer +3) = job_id;

	nbytes = udp_send(local_id,src,send_buffer,nbytes,
                                UDP_CANCEL_JOB_CLI_PORT_NUMBER);
#endif

        }else{
        fprintf(stderr,"cancel job request read error:%s\n",strerror(errno));
        exit(1);
        }//if

        }//while

}//void

void job_monitor_utransfer(){
	//merge the service for UDP_JOB_QUE_SRV_PORT_NUMBER
	int portnumber = UDP_JOB_MONITOR_SRV_PORT_NUMBER;
	CPPTimers timers(4);
	timers.setTimer(0,3000);
	timers.trigger(0);

	char recv_buffer[MSG_BUFF_SIZE];

	UTransfer *transfer = UTransfer::get_instance();
	transfer->init_tcp(portnumber);
	list<upoll_t> src;
	list<upoll_t> dst;
	list<upoll_t>::iterator it;
	USocket*tcp_listener = transfer->get_tcp_listener();
	upoll_t ls_poll;
	ls_poll.events = UPOLL_READ_T;
	ls_poll.pointer = NULL;
	ls_poll.usock = tcp_listener;
	src.push_back(ls_poll);
	map<USocket *,upoll_t> m_tcp_map;
	struct timeval base_tm = {0,100};
	struct timeval wait_tm;
	signal(SIGPIPE, SIG_IGN);
	while(true)
	{
		dst.clear();
		src.clear();
		map<USocket*, upoll_t>::iterator mit;
		for(mit = m_tcp_map.begin();mit!=m_tcp_map.end();mit++)
			src.push_back(mit->second);
		src.push_back(ls_poll);
		
		wait_tm = base_tm;
		int res = transfer->select(dst,src,&wait_tm);
		if(res>0)
		{
			for(it = dst.begin();it!=dst.end();it++)
			{
				upoll_t up = *it;
				if(up.usock == tcp_listener)
				{
					if(up.events&UPOLL_READ_T)
					{
						USocket *sc = transfer->accept(tcp_listener);
						if(sc)
						{	
						upoll_t new_up;
						new_up.pointer = NULL;
						new_up.usock = sc;
						new_up.events = UPOLL_READ_T;
						m_tcp_map[sc] = new_up;
						}
					}
				}
				else if(up.events & UPOLL_READ_T)
				{
		int recv_size = it->usock->recv(recv_buffer,MSG_BUFF_SIZE,NULL);
				if(recv_size>MSG_BUFF_SIZE){
					cerr<<"FINISHED:connection closed."<<endl;
					transfer->destroy_socket(it->usock);
					m_tcp_map.erase(it->usock);	
				}else if(recv_size<0 &&it->usock->get_reason() != EAGAIN){
					cerr<<"SEND ERROR."<<endl;
					transfer->destroy_socket(it->usock);
					m_tcp_map.erase(it->usock);
				}else if(up.events & UPOLL_ERROR_T){
					cerr<<"SYSTEM ERROR:connection closed."<<endl;
					transfer->destroy_socket(it->usock);
					m_tcp_map.erase(it->usock);
				}else if(recv_size<=MSG_BUFF_SIZE&&recv_size>0){

				int type 	= *((int *)recv_buffer);
        			int src  	= *((int *)recv_buffer+1);
        			int dist 	= *((int *)recv_buffer+2);
        			int job_id 	= *((int *)recv_buffer+3);

				char *send_buffer = (char *)malloc(MSG_HEAD_SIZE*sizeof(int));

				switch(type){

				case MSG_TYPE_JOB_CANCEL:
				{
				pthread_mutex_lock(&jobq_lock);
				//batch_job_info *job = &(jobq[job_id]);
				fprintf(stderr,"(job monitor thread) cancel job job_id:%d\n",job_id);	
				for(int node_id = 1;node_id<=NUM_HOSTS;node_id++){
				fprintf(stderr,"#i:%d job_id:%d nodesstat:%d\n",
							node_id,job_id,nodesstat[node_id]);
					if(nodesstat[node_id] == job_id){
					server_single_job_cancel(job_id,node_id);
					}//if
				}//for
				pthread_mutex_unlock(&jobq_lock);
				break;
				}
				//get the job submitted from the client.
				case MSG_TYPE_JOB_SUBMIT:
				{	
				//char *recv_buffer = (char *)malloc(MSG_BUFF_SIZE);
        			//char *send_buffer = (char *)malloc(MSG_HEAD_SIZE*sizeof(int));
        			//int portnumber = UDP_JOB_QUE_SRV_PORT_NUMBER;
        			fprintf(stderr,"job queue push (job_monitor_thread).\n");

        			pthread_mutex_lock(&jobq_lock);
        			//the job queue is full
                		if((job_head+1)%MAX_JOB_QUEUE == job_tail){
                        		pthread_mutex_unlock(&jobq_lock);
                                	fprintf(stderr,"warning: job queue is full.\n");
                                }//if
				//todo if job queue is full then return the failed submit information.
                                pthread_mutex_unlock(&jobq_lock);
                                //portnumber = UDP_JOB_QUE_SRV_PORT_NUMBER;
                                //nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);

                		int type = *((int *)recv_buffer);
                		int src  = *((int *)recv_buffer+1);
                		int dist = *((int *)recv_buffer+2);
                		int comm_str_len  = *((int *)recv_buffer+3);
                		int num_req_nodes = *((int *)recv_buffer+4);
                		int username_len  = *((int *)recv_buffer+5);

                		char *comm_str = (char *)((int *)recv_buffer+MSG_HEAD_SIZE);
                		char *username = (char *)((int *)recv_buffer+MSG_HEAD_SIZE )+comm_str_len;
				//todo
				fprintf(stderr,"job queue push (job monitor thread) read %d bytes command:%s n_nodes:%d user:%s\n",
                                recv_size, comm_str, num_req_nodes, username);
                		//push job into queue;
                                pthread_mutex_lock(&jobq_lock);
                                batch_job_info  *job = &(jobq[job_head]);
                                if(job == NULL){
                                   	fprintf(stderr,"error job queue push job == NULL\n");
                                  	exit(1);
                              	}//if
				sprintf((char *)(job->pcomm),comm_str,comm_str_len);
                		sprintf((char *)(job->username),username,username_len);
                		job->num_req_nodes = num_req_nodes;
                		job->time1 = time(NULL);
                		job->job_stat = JOB_STAT_INQUEUE;
				
				int new_job_id_ = job_head;
                		job->job_id = new_job_id_;
                		job_head = (job_head+1)%MAX_JOB_QUEUE;
                		pthread_mutex_unlock(&jobq_lock);

				portnumber = UDP_JOB_QUE_CLI_PORT_NUMBER;
                		*((int *)send_buffer) = MSG_TYPE_JOB_SUBMIT;
                		*((int *)send_buffer+1) = local_id;
                		*((int *)send_buffer+2) = src;
                		*((int *)send_buffer+4) = new_job_id_;
	fprintf(stderr,"job queue push done. new job:%d job_h:%d job_t:%d\n",new_job_id_,job_head,job_tail);
                		udp_send(local_id,src,send_buffer,MSG_HEAD_SIZE*sizeof(int),portnumber);	
				break;
				}
		//there is no reason why need two switch clause here.				
				//switch(type){
				
				case MSG_TYPE_JOB_QUERY:
				{
				int type = *((int *)recv_buffer);
				int src  = *((int *)recv_buffer+1);
				int dist = *((int *)recv_buffer+2);
				int job_id = *((int *)recv_buffer+3);	
				batch_job_info *job = NULL;
				int cur_job_stat = JOB_STAT_UNKNOW;
				int nbytes;
				if(job_id>=0){
				pthread_mutex_lock(&jobq_lock);
				for(int i = 0;i<MAX_JOB_QUEUE;i++){
					job=&(jobq[i]);
					if(job_id == job->job_id){
						cur_job_stat=job->job_stat;
						break;
					}//if
				}//for
				pthread_mutex_unlock(&jobq_lock);
				//char *send_buffer = (char *)malloc(MSG_HEAD_SIZE*sizeof(int));
				*((int *)send_buffer) = type;
				*((int *)send_buffer+1) = local_id;
				*((int *)send_buffer+2) = src;
				*((int *)send_buffer+3) = job_id;
				*((int *)send_buffer+4) = cur_job_stat;
				portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
				nbytes = udp_send(local_id,src,send_buffer,
					sizeof(int)*MSG_HEAD_SIZE,portnumber);
				}//if

				if(job_id == -1){
				int jobstat[MAX_JOB_QUEUE];
				pthread_mutex_lock(&jobq_lock);
				for(int i=0;i<MAX_JOB_QUEUE;i++){
					job=&(jobq[i]);
					jobstat[i] = job->job_stat;
				}//for
				pthread_mutex_unlock(&jobq_lock);
				char *send_buffer2 = (char *)malloc(MSG_HEAD_SIZE*sizeof(int)
								+MAX_JOB_QUEUE*sizeof(int));
				*((int *)send_buffer2)= type;
				*((int *)send_buffer2+1) = local_id;
				*((int *)send_buffer2+2) = src;
				*((int *)send_buffer2+3) = job_id;
				*((int *)send_buffer2+4) = 0;
				for(int i = 0;i<MAX_JOB_QUEUE;i++){
					*((int *)send_buffer2+MSG_HEAD_SIZE+i)=jobstat[i];
				}//for
				portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
				nbytes = udp_send(local_id,src,send_buffer2,
					sizeof(int)*MSG_HEAD_SIZE+sizeof(int)*MAX_JOB_QUEUE,portnumber);
				}//if(job_id == -1)
				if(job_id == -2){
					batch_job_info jobi[MAX_JOB_QUEUE];
					batch_job_info *pjob;
					char *send_buffer3 = (char *)malloc(MSG_HEAD_SIZE*sizeof(int)+
					MAX_JOB_QUEUE*sizeof(batch_job_info));
					pthread_mutex_lock(&jobq_lock);
					for(int i = 0;i<MAX_JOB_QUEUE;i++){
						pjob = &(jobq[i]);
					batch_job_info *ptmp = (batch_job_info *)(send_buffer3+
					sizeof(int)*MSG_HEAD_SIZE+i*sizeof(batch_job_info));
					struct tm *tmt;
					time_t t = time(NULL);
					tmt = localtime(&t);
					memcpy((char *)ptmp,(char *)pjob,sizeof(batch_job_info));
					//asctime(tmt);
	fprintf(stderr,"debug job id:%d stat:%d time0:%d nodes:%d user0:%s user1:%s\n",
		pjob->job_id,pjob->job_stat,pjob->time0,pjob->num_req_nodes,
			pjob->username,ptmp->username);
					}
					pthread_mutex_unlock(&jobq_lock);
					portnumber=UDP_JOB_STAT_CLI_PORT_NUMBER;
					nbytes = udp_send(local_id, src,send_buffer3,
		sizeof(int)*MSG_HEAD_SIZE+sizeof(batch_job_info)*MAX_JOB_QUEUE,portnumber);
				}//job_id == -2
				break;
				}//case msg_type_job_query
				}//switch(type)
				}
				}
			}//for
		}//if(res>0)
	}//while
}

//depricated function to be (*removed*) in new version
void job_monitor(){
	
	int portnumber;
	char *send_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	char *recv_buffer = (char *)malloc(MSG_BUFF_SIZE);
	
	while(1){
	
	portnumber = UDP_JOB_MONITOR_SRV_PORT_NUMBER;
	int num_req_nodes = 0;
	int i;
	int isavailable;
	int server_machine_id = 1;

	int nbytes;
	nbytes = udp_recv(local_id,server_machine_id,recv_buffer,
						MSG_BUFF_SIZE,portnumber);

	int type = *((int *)recv_buffer);
	int src  = *((int *)recv_buffer+1);
	int dist = *((int *)recv_buffer+2);
	int job_id = *((int *)recv_buffer+3);

	switch(type){

	case MSG_TYPE_JOB_CANCEL:

	pthread_mutex_lock(&jobq_lock);
	batch_job_info *job = &(jobq[job_id]);
	
        int type = *((int *)recv_buffer);
        int src  = *((int *)recv_buffer+1);
        int dist = *((int *)recv_buffer+2);
        int job_id = *((int *)recv_buffer+3);

	fprintf(stderr,"job monitor act:cancel src:%d dist:%d job_id:%d\n",src,dist,job_id);

        int i;
        int host_id = 0;
        for(i=1;i<=NUM_HOSTS;i++){
	fprintf(stderr,"#i:%d job_id:%d nodesstat:%d\n",i,job_id,nodesstat[i]);
                if(nodesstat[i] == job_id){
                server_single_job_cancel(job_id,i);
                }//server_single_job_cancel
        }//for

	pthread_mutex_unlock(&jobq_lock);

	break;
	
	//case MSG_TYPE_JOB_SUBMIT:

	//break;	
	//default:
	
	}//switch	

	}//while
}//

//depricated function to be (*removed*) in new version.
void job_status_query(){

	int portnumber;
	char pcomm[32];
	char *recv_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	char *send_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);

	while(1){
	portnumber = UDP_JOB_STAT_SRV_PORT_NUMBER;
	
	int num_req_nodes = 0;
	int i;
	int isavailable;
	int server_machine_id = 1;
	
	int nbytes; 
	nbytes = udp_recv(local_id,server_machine_id,recv_buffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);
	int type = *((int *)recv_buffer);
	int src  = *((int *)recv_buffer+1);
	int dist = *((int *)recv_buffer+2);
	int job_id = *((int *)recv_buffer+3);

	fprintf(stderr,"job stat query type:%d src:%d dist:%d job_id:%d\n",type,src,dist,job_id);
	
	batch_job_info *job = NULL;
	int cur_job_stat = JOB_STAT_UNKNOW;

	if(job_id >= 0){
	pthread_mutex_lock(&jobq_lock);
	for(i=0;i<MAX_JOB_QUEUE;i++){
		job = &(jobq[i]);
		if(job_id == job->job_id){
			cur_job_stat = job->job_stat;	
			break;
		}//if
	}//for
	pthread_mutex_unlock(&jobq_lock);
	*((int *)send_buffer) = type;
	*((int *)send_buffer+1) = local_id;
	*((int *)send_buffer+2) = src;
	*((int *)send_buffer+3) = job_id;
	*((int *)send_buffer+4) = cur_job_stat;
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;	
	nbytes = udp_send(local_id,src,send_buffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);
	//fprintf(stderr,"job stat query udp_send %d bytes\n",nbytes);
	
	}//if	

	if(job_id == -1){
	int jobstat[MAX_JOB_QUEUE];
	pthread_mutex_lock(&jobq_lock);
	for(i=0;i<MAX_JOB_QUEUE;i++){
		job = &(jobq[i]);
		jobstat[i] = job->job_stat;
		//fprintf(stderr,"\t job_id:%d job_stat:%d\n",job->job_id,job->job_stat);
	}//for
	pthread_mutex_unlock(&jobq_lock);
	char *send_buffer2 = (char *)malloc(MSG_HEAD_SIZE*sizeof(int)+MAX_JOB_QUEUE*sizeof(int));
	*((int *)send_buffer2) = type;
	*((int *)send_buffer2+1) = local_id;
	*((int *)send_buffer2+2) = src;
	*((int *)send_buffer2+3) = job_id;
	*((int *)send_buffer2+4) = 0;
	for(i=0;i<MAX_JOB_QUEUE;i++){
		*((int *)send_buffer2+MSG_HEAD_SIZE+i) = jobstat[i];
	}//for
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
	nbytes = udp_send(local_id,src,send_buffer2,
				sizeof(int)*MSG_HEAD_SIZE+sizeof(int)*MAX_JOB_QUEUE,portnumber);
	//fprintf(stderr,"job stat query udp_send %d bytes\n",nbytes);
	}//if job_id == -1

	if(job_id == -2){

	batch_job_info jobi[MAX_JOB_QUEUE];
	batch_job_info *pjob;
	char *send_buffer3 = (char *)malloc(MSG_HEAD_SIZE*sizeof(int)+
						MAX_JOB_QUEUE*sizeof(batch_job_info));
	pthread_mutex_lock(&jobq_lock);
	for(i=0;i<MAX_JOB_QUEUE;i++){
		pjob = &(jobq[i]);
	batch_job_info * ptmp = (batch_job_info *)(send_buffer3 + sizeof(int)*MSG_HEAD_SIZE+i*sizeof(batch_job_info)); 	
	
	struct tm *tmt;
	time_t t = time(NULL);
	tmt = localtime(&t);
	memcpy((char *)ptmp,(char *)pjob,sizeof(batch_job_info));
	//asctime(tmt),
	fprintf(stderr,"debug job id:%d stat:%d time0:%d nodes:%d user0:%s user1:%s\n",
		pjob->job_id,pjob->job_stat,pjob->time0,pjob->num_req_nodes,
			pjob->username,ptmp->username);

	}//for
	pthread_mutex_unlock(&jobq_lock);
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
	nbytes = udp_send(local_id,src,send_buffer3,
			sizeof(int)*MSG_HEAD_SIZE+sizeof(batch_job_info)*MAX_JOB_QUEUE,portnumber);
	}//if job_id == -2

	}//while

}//void

void job_queue_result(){

	int portnumber = UDP_JOB_RELT_SRV_PORT_NUMBER;
	int count = 0;
	fprintf(stderr,"job queue result thread portnumber:%d\n",portnumber);
	char *recv_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	int nbytes;

	while(1){

	nbytes = udp_recv(local_id,0,recv_buffer,MSG_HEAD_SIZE*sizeof(int),portnumber);
	if(nbytes != MSG_HEAD_SIZE*sizeof(int)){
		fprintf(stderr,"error job queue result udp_recv nbytes!=MSG_HEAD_SIZE*sizeof(int)\n");
		exit(1);
	}//if

	int type = *((int *)recv_buffer);
	int src  = *((int *)recv_buffer+1);
	int dist = *((int *)recv_buffer+2);
	int job_id = *((int *)recv_buffer+3);
	int job_relt = *((int *)recv_buffer+4); 	
	//fprintf(stderr,"job_id:%d job_relt:%d\n",job_id,job_relt);

	if(job_relt == JOB_STAT_FINISHED){
	pcstat[src] = HOST_STAT_FREE;
	jobnodes[job_id]--;
	if(jobnodes[job_id] == 0){
		pthread_mutex_lock(&jobq_lock);
		jobq[job_id].job_stat = JOB_STAT_FINISHED;	
		jobq[job_id].time3 = time(NULL);
		pthread_mutex_unlock(&jobq_lock);
	}//if(jobnodes[job_id] == 0)

	}//if
	
	}//while
}


//FIFO
void job_queue_pop(){

	int count = 0;
	fprintf(stderr,"job queue pop thread portnumber:0\n");
			
	char pcomm[32];
	int num_req_nodes = 0;
	int machine_id;
	int i;
	int isavailable;
	int job_id = 0;
	for(i=0;i<MAX_NUM_HOSTS;i++){
		pcstat[i] = HOST_STAT_FREE;
	}//for

	int iter = 0;
	while(1){

	batch_job_info *job = NULL;
	sleep(1);

	if(iter % 5 == 0){
	fprintf(stderr,"job queue pop job_head:%d job_tail:%d\n",job_head,job_tail);
	sleep(1);
	}
	iter++;

	pthread_mutex_lock(&jobq_lock);
	if(job_head == job_tail)//the job queue is empty
	{
		pthread_mutex_unlock(&jobq_lock);
		sleep(1);
		continue;
	}//if

	job = &(jobq[job_tail]);

	if(job == NULL){
		fprintf(stderr,"error job queue pop job == NULL\n");
		exit(1);
	}//if

	job_id = job->job_id;
	sprintf(pcomm, job->pcomm, strlen(job->pcomm));
	num_req_nodes = job->num_req_nodes;
	fprintf(stderr,"job queue pop pcomm:%s req_nodes:%d job_id:%d\n",
								pcomm,num_req_nodes,job_id);
	job_tail = (job_tail+1)%MAX_JOB_QUEUE;
	job->job_stat = JOB_STAT_RUNNING;
	pthread_mutex_unlock(&jobq_lock);

	count = 0;
	while(1){
	//count = 0;
        for(i= 1;i<=NUM_HOSTS;i++){
		if(pcstat[i] == HOST_STAT_FREE)
			count++;
        }//for
	//if(count ==0){
	//	sleep(2);
	//	continue;
	//}//if
	if(count<num_req_nodes){
		sleep(1);
		continue;
	}//if
	if(count>=num_req_nodes)
		break;

	}//while

	job->time2 = time(NULL);

        int k = 1;
        for(i=1;i<=num_req_nodes;i++){
                for(k=1;k<=NUM_HOSTS;k++){
			fprintf(stderr,"pcstat[%d]=(%d) FREE(%d) job_id:%d\n",
				k,pcstat[k],HOST_STAT_FREE,job_id);
			if(pcstat[k] == HOST_STAT_FREE){
			nodesstat[k] 	= job_id;
			pcstat[k] 	= HOST_STAT_BUSY;
          		server_single_job_submit(local_id,k,pcomm,job_id);
                        break;
                        }
		}//for
        }//for
	jobnodes[job_id] = num_req_nodes;
	fprintf(stderr,"#job queue (pop) num_req_nodes:%d job id:%d comm:%s",
						num_req_nodes, job_id, pcomm);
	/*for(k=1;k<=NUM_HOSTS;k++){
		if(nodesstat[k] == job_id)
			fprintf(stderr," ,%d",k);
	}//for
	fprintf(stderr,"\n");*/
	}//while
}//void

int server_query_host_stat(int machine_id){

#if 0	
#endif
	return 0;
}//void

void server_single_job_submit(int local_id, int remote_id, char *command, int job_id){

        int portnumber = UDP_INTER_JOB_SRV_PORT_NUMBER;
        int len = strlen(command);

        int type = MSG_TYPE_JOB_SUBMIT;
        int src  = local_id;
        int dist = remote_id;  //ANY remote pc
        len = len + 1;
        int pcomm_len = len;
        int nbytes;
        int job_msg_len = pcomm_len+MSG_HEAD_SIZE*sizeof(int);
        char *sendbuffer = (char *)malloc(job_msg_len);

	fprintf(stderr,"server_single_job_submit remote_id:%d pcomm:%s job_id:%d\n",
				remote_id,command,job_id);

        memcpy(sendbuffer, (char *)&type, sizeof(int));
        memcpy(sendbuffer+1*sizeof(int),(char *)&src,sizeof(int));
        memcpy(sendbuffer+2*sizeof(int),(char *)&dist,sizeof(int));
        memcpy(sendbuffer+3*sizeof(int),(char *)&job_id,sizeof(int));
        memcpy(sendbuffer+4*sizeof(int),(char *)&pcomm_len,sizeof(int));
        memcpy(sendbuffer+MSG_HEAD_SIZE*sizeof(int),(char *)command,pcomm_len);

        nbytes = udp_send(src,dist,sendbuffer,job_msg_len,portnumber);
		
        if((nbytes == -1)){
                fprintf(stderr,"error server_single_job_submit udp_send error:%s\n",strerror(errno));
		//jobnodes[job_id]--;
                exit(1);
        }//fi
        if(nbytes != job_msg_len){
        	fprintf(stderr,"error server_single_job_submit(%s) nbytes!=job_msg_len\n",command);
		exit(1);
	}//if
}//void

void server_single_job_cancel(int job_id_, int host_id_){
	
	fprintf(stderr,"server_single_job_cancel job_id:%d host_id:%d\n",job_id_,host_id_);
		
	int portnumber = UDP_INTER_JOB_SRV_PORT_NUMBER;
	portnumber = UDP_CANCEL_JOB_SRV_PORT_NUMBER;

	int type = MSG_TYPE_JOB_CANCEL;
	int src  = local_id;
	int dist = host_id_;
	int job_id = job_id_;
	
	char *sendbuffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	memcpy(sendbuffer,(char *)&type,sizeof(int));
	memcpy(sendbuffer+1*sizeof(int),(char *)&src,sizeof(int));
	memcpy(sendbuffer+2*sizeof(int),(char *)&dist,sizeof(int));
	memcpy(sendbuffer+3*sizeof(int),(char *)&job_id,sizeof(int));
	int nbytes = udp_send(local_id,dist,sendbuffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);
	
}

//FIFO
//(depricated function) will be (*removed*) in new version; 2015/10/02
void job_queue_push(){

	int count = 0;
	int new_job_id;
	int nbytes;

	char *recv_buffer = (char *)malloc(MSG_BUFF_SIZE);
	char *send_buffer = (char *)malloc(MSG_HEAD_SIZE*sizeof(int));
	int portnumber = UDP_JOB_QUE_SRV_PORT_NUMBER;
	
	fprintf(stderr,"job queue push thread portnumber:%d\n",portnumber);

	while(1){

	pthread_mutex_lock(&jobq_lock);
	//the job queue is full
	if((job_head+1)%MAX_JOB_QUEUE == job_tail){
	pthread_mutex_unlock(&jobq_lock);
	fprintf(stderr,"warning: job queue full\n");
	sleep(2);	
	continue;
	}//if
	pthread_mutex_unlock(&jobq_lock);

	//portnumber = UDP_JOB_QUE_SRV_PORT_NUMBER;
        nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);

        if(nbytes>0){

                int type = *((int *)recv_buffer);
                int src  = *((int *)recv_buffer+1);
                int dist = *((int *)recv_buffer+2);
                int pcomm_len = *((int *)recv_buffer+3);
                int num_req_nodes = *((int *)recv_buffer+4);
		int username_len = *((int *)recv_buffer+5);
		
                char *pcomm = (char *)((int *)recv_buffer+MSG_HEAD_SIZE);
       		char *username = (char *)((int *)recv_buffer+MSG_HEAD_SIZE )+pcomm_len;
       
	 	//pcomm[payload_len] = '\0';

		fprintf(stderr,"job queue push thread read %d bytes pcommand:%s n_nodes:%d user:%s\n",
                                nbytes, pcomm, num_req_nodes, username);
		//put job into queue;
		pthread_mutex_lock(&jobq_lock);
		batch_job_info 	*job = &(jobq[job_head]);
		if(job == NULL){
		fprintf(stderr,"error job queue push job == NULL\n");
		exit(1);
		}//if

		sprintf((char *)(job->pcomm),pcomm,pcomm_len);
		sprintf((char *)(job->username),username,username_len);

		job->num_req_nodes = num_req_nodes;
		job->time1 = time(NULL);
		job->job_stat = JOB_STAT_INQUEUE;
		
		new_job_id = job_head;
		job->job_id = new_job_id;
		job_head = (job_head+1)%MAX_JOB_QUEUE;
		pthread_mutex_unlock(&jobq_lock);

		portnumber = UDP_JOB_QUE_CLI_PORT_NUMBER;
		*((int *)send_buffer) = MSG_TYPE_JOB_SUBMIT;
		*((int *)send_buffer+1) = local_id;
		*((int *)send_buffer+2) = src;
		*((int *)send_buffer+4) = new_job_id;

		fprintf(stderr,"job queue push done. new job:%d job_h:%d job_t:%d\n",
					new_job_id, job_head, job_tail);
		udp_send(local_id,src,send_buffer,MSG_HEAD_SIZE*sizeof(int),portnumber);

	}//if

	}//while

}//void
