
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
//#include "utransfer.h"
//#include "rusocket.h"
#include "datatype.h"
#include "eTransfer.h"

#if 0
HOST_STAT_FREE 
HOST_STAT_BUSY
HOST_STAT_ONLINE 
HOST_STAT_OFFLINE
#endif

extern epbs_host_info hostq[(MAX_NUM_HOSTS+1)];
extern pthread_mutex_t hostq_lock;

//version 2 uses the etransfer
void host_monitor_transfer_2(){
	int portnumber = UDP_HOST_MONITOR_SRV_PORT_NUMBER;
	fprintf(stderr,"(host_monitor_transfer_1:%d)\n",portnumber);
	//int type = MSG_TYPE_HOST;
	//int src = local_id;
	//int dist = 0;
	//int machine_id = 0;
	//int server_machine_id = 1;
	int send_buffer_len = sizeof(int)*MSG_HEAD_SIZE;
	char *send_buffer = NULL;//(char *)malloc(send_buffer_len);
	char *recv_buffer = (char *)malloc(MSG_BUFF_SIZE);
	int recv_size = 0;
	int send_size = 0; 	

	eTransfer *transfer = new eTransfer();
	transfer->init_tcp(4323);
	UTcpSocket *tcp_listener = transfer->get_tcp_listener();
	list<upoll_t> src;
	list<upoll_t> dst;
	list<upoll_t>::iterator it;
	upoll_t ls_poll;
	ls_poll.events = UPOLL_READ_T;
	ls_poll.pointer = NULL;
	ls_poll.usock = tcp_listener;
	src.push_back(ls_poll);
	map<USocket*,upoll_t>m_tcp_map;
	struct timeval base_tm = {0,100};
	struct timeval wait_tm;
	while(true)
	{
		dst.clear();
		src.clear();
		map<USocket*,upoll_t>::iterator mit;
		for(mit = m_tcp_map.begin();mit!=m_tcp_map.end();mit++)
			src.push_back(mit->second);
		src.push_back(ls_poll);
		wait_tm = base_tm;
		transfer->walk_through();
		int res = transfer->select(dst,src,&wait_tm);
		if(res>0)
		{
			for(it=dst.begin();it!=dst.end();it++)
			{
				upoll_t up = *it;
				if(up.usock == tcp_listener)
				{
					if(up.events&UPOLL_READ_T)
					{
						USocket *sc = transfer->accept(tcp_listener);
						if(sc)
						{
							cerr<<"transfer->accept"<<endl;
							upoll_t new_up;
							new_up.pointer = NULL;
							new_up.usock = sc;
							new_up.events = UPOLL_READ_T;
							m_tcp_map[sc] = new_up;
						}//if
					}
				}else if(up.events & UPOLL_READ_T)
				{
					//char data[MSG_BUFF_SIZE];
					recv_size = it->usock->recv(recv_buffer,MSG_BUFF_SIZE,NULL);
					if(recv_size>0){
					upoll_t write_up;
					write_up.usock = it->usock;
					send_buffer = (char *)malloc(sizeof(char)*MSG_BUFF_SIZE);
					write_up.pointer = send_buffer;
					write_up.events = UPOLL_WRITE_T;
					m_tcp_map[it->usock] = write_up;
					int type_ = *((int*)recv_buffer);
					int src_ = *((int*)recv_buffer+1);
					int dist_ = *((int*)recv_buffer+2);
					int qh_id = *((int*)recv_buffer+3);
					if(qh_id>=1){
						*((int *)send_buffer) = type_;
						*((int *)send_buffer+1) = local_id;
						*((int *)send_buffer+2) = src_;
						*((int *)send_buffer+3) = qh_id;
						for(int i = 0;i<num_hosts;i++){
						*((int *)send_buffer+MSG_HEAD_SIZE+i)= pcstat[i];
						}
					}//if
					if(qh_id == 0){
						*((int *)send_buffer) = type_;
						*((int *)send_buffer+1) = local_id;
						*((int *)send_buffer+2) = src_;
						*((int *)send_buffer+3) = qh_id;
						pthread_mutex_lock(&hostq_lock);
						for(int i = 0;i<num_hosts;i++){
						epbs_host_info *phost = (epbs_host_info *)(send_buffer+
						sizeof(int)*MSG_HEAD_SIZE+sizeof(epbs_host_info)*i);
						phost->host_stat = hostq[i].host_stat;
						phost->host_id = i+1;
						strncpy(phost->ip,hostq[i].ip,32);
						}//for
						pthread_mutex_unlock(&hostq_lock);
					}
					}//if
					//fprintf(stderr,"recv_size:%d\n",recv_size);
				}else if(up.events & UPOLL_WRITE_T)
				{
					it->usock->send((char *)(it->pointer),MSG_BUFF_SIZE,NULL);
					free(it->pointer);
					it->pointer = NULL;
					transfer->destroy_socket(it->usock);
					m_tcp_map.erase(it->usock);
				}else if(up.events & UPOLL_ERROR_T)
				{
					cerr<<"SYSTEM ERROR"<<endl;
					continue;
				}
			}
		}//if(res>0)		
	}//while(true)
	return;
}//

//version 1 uses the UTcpSocket
void host_monitor_transfer_1(){
	//fprintf(stderr,"host_monitor_transfer_1\n");	
	int portnumber = UDP_HOST_MONITOR_SRV_PORT_NUMBER;
	fprintf(stderr,"(host_monitor_transfer_1:%d)\n",portnumber);	
	int type = MSG_TYPE_HOST;
	int src = local_id;
	int dist = 0;
	int machine_id = 0;
	int server_machine_id = 1;
	int send_buffer_len = sizeof(int)*MSG_HEAD_SIZE;
	char *send_buffer = (char *)malloc(send_buffer_len);
	char *recv_buffer = (char *)malloc(MSG_BUFF_SIZE);
	int recv_size = 0;
	int send_size = 0;
	memcpy(send_buffer,(char *)&type,sizeof(int));
	memcpy(send_buffer+1*sizeof(int),(char *)&src,sizeof(int));
	memcpy(send_buffer+2*sizeof(int),(char *)&dist,sizeof(int));
	memcpy(send_buffer+3*sizeof(int),(char *)&machine_id,sizeof(int));
	//UTransfer *transfer = UTransfer::get_instance();
	
	char *hip = get_host_ip(server_machine_id);
	if(hip == NULL) return;
	int m_tcp_port = portnumber;
	int m_tcp_sock = socket(PF_INET,SOCK_STREAM,0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(portnumber);
	if(bind(m_tcp_sock,(sockaddr *)&addr,sizeof(addr))<0)
	{
		cerr<<"(bind error)"<<endl;
		return;
	}//if
	if(listen(m_tcp_sock,5)<0){
		cerr<<"listen error."<<endl;
		return;
	}//if
	socklen_t addrlen = sizeof(addr);
	int new_sock = ::accept(m_tcp_sock,(struct sockaddr*)&addr,
		&addrlen);
	if(new_sock>=0)
	{
		UTcpSocket *usocket = new UTcpSocket();
		CSockAddr caddr(addr);
		usocket->bind(new_sock,caddr);
		fprintf(stderr,"(::accept success)\n");
		recv_size = usocket->recv(recv_buffer,MSG_BUFF_SIZE,NULL);
		fprintf(stderr,"(recv:%d)\n",recv_size);
		if(recv_size>0){
		int type = *((int *)recv_buffer);
		int src = *((int *)recv_buffer+1);
		int dist = *((int *)recv_buffer+2);
		int qh_id = *((int *)recv_buffer+3);
		if(qh_id >= 1){
			*((int *)send_buffer) = type;
			*((int *)send_buffer+1) = local_id;
			*((int *)send_buffer+2) = src;
			*((int *)send_buffer+3) = qh_id;
			for(int i = 0;i<num_hosts;i++){
		*((int *)send_buffer+MSG_HEAD_SIZE+i) = pcstat[i];
			}//for
		}//if(qh_id == 0)
		if(qh_id == 0){
			*((int *)send_buffer) = type;
			*((int *)send_buffer+1) = local_id;
			*((int *)send_buffer+2) = src;
			*((int *)send_buffer+3) = qh_id;
			pthread_mutex_lock(&hostq_lock);
			for(int i = 0;i<num_hosts;i++){
			epbs_host_info *phost = (epbs_host_info *)(send_buffer+
			sizeof(int)*MSG_HEAD_SIZE+sizeof(epbs_host_info)*i);
			phost->host_stat = hostq[i].host_stat;
			phost->host_id = i+1;
			strncpy(phost->ip,hostq[i].ip,32);	
			}//for
			pthread_mutex_unlock(&hostq_lock);	
		}	//if

		if(usocket->can_write()){
	send_size = usocket->send(send_buffer,MSG_BUFF_SIZE,NULL);
		fprintf(stderr,"can_write() send_size:%d\n",send_size);
		}//if
	
		}//if (recv nbytes>0)
	}
}

//version 0 uses the utransfer
//depricated function, uses etransfer instead.
void host_monitor_transfer_0(){
#if 0	
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
	//unsigned short port = portnumber;
	
	transfer->init_tcp(portnumber);
	list<upoll_t>src;
	list<upoll_t>dst;
	list<upoll_t>::iterator it;
	USocket*tcp_listener = transfer->get_tcp_listener();
	upoll_t ls_poll;
	ls_poll.events = UPOLL_READ_T;
	ls_poll.pointer = NULL;
	ls_poll.usock = tcp_listener;
	src.push_back(ls_poll);
	map<USocket*,upoll_t> m_tcp_map;
	struct timeval base_tm = {0,100};
	struct timeval wait_tm;
	signal(SIGPIPE,SIG_IGN);
	fprintf(stderr,"debug host_monitor_transfer_0 portnumber:%d\n",portnumber);
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
			fprintf(stderr,"debug transfer->select\n");
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
							cerr<<"debug host_monitor accept socket."<<endl;
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
							for(int i = 0;i<num_hosts;i++){
								*((int *)send_buffer+MSG_HEAD_SIZE+i) = pcstat[i];
							}
						}//if(qh_id == 0)
						if(qh_id == 0){
							*((int *)send_buffer) = type;
							*((int *)send_buffer+1) = local_id;
							*((int *)send_buffer+2) = src;
							*((int *)send_buffer+3) = qh_id;
							pthread_mutex_lock(&hostq_lock);
							for(int i = 0;i<num_hosts;i++){
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
#endif
}

//depricated function to be (*removed*) in new version
void host_query(){
#if 0
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
#endif
}//host_query
