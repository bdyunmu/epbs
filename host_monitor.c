
//host component for easy pbs v0.2
//www.bdyunmu.com
//9/16/2015

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>

#include "libhost.h"
#include "libmsg.h"
#include "PPTimers.h"
#include "utransfer.h"
#include "rusocket.h"
#include "datatype.h"

#if 0
HOST_STAT_FREE 
HOST_STAT_BUSY 
HOST_STAT_ONLINE 
HOST_STAT_OFFLINE
#endif

extern epbs_host_info hostq[(MAX_NUM_HOSTS+1)];
extern pthread_mutex_t hostq_lock;

void host_monitor_transfer_0(){
	int portnumber = UDP_HOST_MONITOR_SRV_PORT_NUMBER;
	CPPTimers timers(4);
	timers.setTimer(0,3000);
	timers.trigger(0);
	char recv_buffer[MSG_BUFF_SIZE];
	char send_buffer[MSG_BUFF_SIZE];
	int recv_size = 0;
	int send_size = 0;
	//todo need to study the global static member 
	UTransfer *transfer = UTransfer::get_instance();
	unsigned short port = portnumber;
	transfer->init_tcp(port);
	
	list<upoll_t>src;
	list<upoll_t>dst;
	list<upoll_t>::iterator it;
	USocket*tcp_listener = transfer->get_tcp_listener();
	upoll_t ls_poll;
	ls_poll.events = UPOLL_READ_T;
	ls_poll.pointer = (void *)tcp_listener;
	ls_poll.usock = tcp_listener;
	src.push_back(ls_poll);
	map<USocket*,upoll_t> m_tcp_map;
	struct timeval base_tm = {0,100};
	struct timeval wait_tm;
	signal(SIGPIPE,SIG_IGN);
	while(true)
	{
		dst.clear();
		src.clear();
		map<USocket*,upoll_t>::iterator mit;
		for(mit = m_tcp_map.begin();mit!=m_tcp_map.end();mit++)
			src.push_back(mit->second);
		src.push_back(ls_poll);
		wait_tm =base_tm;
		int res = transfer->select(dst,src,&wait_tm);

		if(res>0)
		{
			for(it=dst.begin();it!=dst.end();it++)
			{
				upoll_t up = *it;
				if(up.usock==tcp_listener)
				{
					if(up.events & UPOLL_READ_T)
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
				}//up.usock == tcp_listener
				else if(up.events & UPOLL_WRITE_T){
				it->usock->send((char *)(it->pointer), MSG_BUFF_SIZE, NULL);
				free(it->pointer);
				it->pointer = NULL;
			   	transfer->destroy_socket(it->usock);
                                m_tcp_map.erase(it->usock);
				}
				else if(up.events & UPOLL_READ_T)
				{
						int recv_size = it->usock->recv(recv_buffer,MSG_BUFF_SIZE,NULL);
						if(recv_size>0){
						char *send_buffer = (char *)malloc(sizeof(char)*MSG_BUFF_SIZE);	
						upoll_t write_up;
						write_up.usock = it->usock;
						write_up.pointer = send_buffer;
						write_up.events = UPOLL_WRITE_T;
						m_tcp_map[it->usock] = write_up;		
						int type  = *((int *)recv_buffer);
						int src   = *((int *)recv_buffer+1);
						int dist  = *((int *)recv_buffer+2);
						int qh_id = *((int *)recv_buffer+3);
						if(qh_id>=1){
							*((int *)send_buffer) = type;
							*((int *)send_buffer+1) = local_id;
							*((int *)send_buffer+2) = src;
							*((int *)send_buffer+3) = qh_id;
							for(int i = 0;i<MAX_NUM_HOSTS;i++){
								*((int *)send_buffer+MSG_HEAD_SIZE+i) = pcstat[i];
							}
						}//if(qh_id == 0)
						if(qh_id == 0){
							*((int *)send_buffer) = type;
							*((int *)send_buffer+1) = local_id;
							*((int *)send_buffer+2) = src;
							*((int *)send_buffer+3) = qh_id;
							pthread_mutex_lock(&hostq_lock);
							for(int i = 0;i<MAX_NUM_HOSTS;i++){
							epbs_host_info *phost = (epbs_host_info *)(send_buffer+
								sizeof(int)*MSG_HEAD_SIZE+sizeof(epbs_host_info)*i);
							phost->host_stat = hostq[i].host_stat;
							phost->host_id = i+1;
							strncpy(phost->ip,hostq[i].ip,32);	
							}//for
							pthread_mutex_unlock(&hostq_lock);
						}//if
						}//if(recv_size>0)
				}//else
			}
		}
	}
}

//depricated function to be (*removed*) in new version
void host_query(){

	//fprintf(stderr,"host query thread portnumber:%d\n",
	//		UDP_HOST_QUERY_SRV_PORT_NUMBER);
        int i;
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        int sin_size = 0;
        int portnumber = UDP_HOST_MONITOR_SRV_PORT_NUMBER;
	char recv_buffer[MSG_BUFF_SIZE];
	char send_buffer[MSG_HEAD_SIZE*sizeof(int)+sizeof(int)*MAX_NUM_HOSTS];
       
	while(1){

	portnumber = UDP_HOST_MONITOR_SRV_PORT_NUMBER;
        int nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);
        int stat = 0;
        //pthread_mutex_lock(&hostq_lock);
        //pthread_mutex_unlock(&hostq_lock);
	if(nbytes>0){
		
                int type  = *((int *)recv_buffer);
                int src   = *((int *)recv_buffer+1);
                int dist  = *((int *)recv_buffer+2);
		int qh_id = *((int *)recv_buffer+3);
		
	fprintf(stderr,"host query type:%d src:%d dist:%d qh_id:%d\n",type,src,dist,qh_id);

	if(qh_id >= 1){

	portnumber = UDP_HOST_MONITOR_CLI_PORT_NUMBER;
	*((int *)send_buffer) = type;
	*((int *)send_buffer+1) = local_id;
	*((int *)send_buffer+2) = src;
	*((int *)send_buffer+3) = qh_id;

	for (i=0;i<MAX_NUM_HOSTS;i++){
		*((int *)send_buffer+MSG_HEAD_SIZE+i) = pcstat[i];	
	}//for
	udp_send(local_id,src,send_buffer,
			MSG_HEAD_SIZE*sizeof(int)+sizeof(int)*MAX_NUM_HOSTS,portnumber);
	
	}//if (qh_id >= 1)
	
	if(qh_id == 0){
	
	portnumber = UDP_HOST_MONITOR_CLI_PORT_NUMBER;
	int send_buffer2_len = sizeof(int)*MSG_HEAD_SIZE+sizeof(epbs_host_info)*MAX_NUM_HOSTS;
	char *send_buffer2 = (char *)malloc(send_buffer2_len);

	*((int *)send_buffer2) 	 = type;
	*((int *)send_buffer2+1) = local_id;
	*((int *)send_buffer2+2) = src;
	*((int *)send_buffer2+3) = qh_id;
	
	pthread_mutex_lock(&hostq_lock);
	for (i=0;i<MAX_NUM_HOSTS;i++){

	epbs_host_info *phost = (epbs_host_info *)(send_buffer2+
		sizeof(int)*MSG_HEAD_SIZE+sizeof(epbs_host_info)*i);	
	phost->host_stat = hostq[i].host_stat;
	phost->host_id = i+1;

	strncpy(phost->ip,hostq[i].ip,32);
	}//for
	pthread_mutex_unlock(&hostq_lock);
	nbytes = udp_send(local_id,src,send_buffer2,send_buffer2_len,portnumber);
	
	}//if qh_id >= 1
        
        }else{
        fprintf(stderr,"host query error:%s\n",strerror(errno));
        exit(1);
        }//if

        }//while

	for(i=0;i<MAX_NUM_HOSTS;i++){
		pcstat[i] = HOST_STAT_FREE;
	}//for

}//host_query
