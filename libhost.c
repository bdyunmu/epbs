/*
host component for easy pbs v0.2
copytright belongs to www.bdyunmu.com
author:lihui@indiana.edu
last update: 9/16/2015
*/

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


int  local_id;
char hostname[64];
int  num_hosts;

char hostfile[64];
char *ips[(MAX_NUM_HOSTS+1)];
char *hosts[(MAX_NUM_HOSTS+1)];     //server   hosts
int  pcstat[(MAX_NUM_HOSTS)];	

#if 0
HOST_STAT_FREE 
HOST_STAT_BUSY 
HOST_STAT_ONLINE 
HOST_STAT_OFFLINE
#endif

epbs_host_info hostq[(MAX_NUM_HOSTS+1)];
pthread_mutex_t hostq_lock;
int local_machine_stat;

char *get_host_ip(int dist_id){
	if(dist_id>num_hosts || dist_id<=0) return NULL;
	else return ips[dist_id];
}//char

//help function that maintain the host status received from all the other compute nodes.
void epbs_host_status(){
	
	int portnumber = UDP_HOST_STAT_SRV_PORT_NUMBER;
	eTransfer *transfer = new eTransfer();
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
	map<USocket*,upoll_t>m_tcp_map;
	struct timeval base_tm = {0,100};
	struct timeval wait_tm;	
	fprintf(stderr,"epbs_host_status thread portnumber:%d\n",portnumber);
	int i;
	int recv_size = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int sin_size = 0;
	char recv_buffer[MSG_BUFF_SIZE];

	while(true){
	//portnumber = UDP_HOST_STAT_SRV_PORT_NUMBER;
	//int nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);
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
							upoll_t new_up;
							new_up.pointer = NULL;
							new_up.usock = sc;
							new_up.events = UPOLL_READ_T;
							m_tcp_map[sc] = new_up;	
						}//if(sc)

					}//if
				}else if(up.events & UPOLL_READ_T)
				{
				recv_size = it->usock->recv(recv_buffer,MSG_BUFF_SIZE,NULL);
				transfer->destroy_socket(it->usock);
				m_tcp_map.erase(it->usock);	
				}//else if
			}//for	
		}//if
	
		if(recv_size >0){
		int type  = *((int *)recv_buffer);
		int src   = *((int *)recv_buffer+1);
		int dist  = *((int *)recv_buffer+2);
		int host_id = *((int *)recv_buffer+3);
		int host_status = *((int *)recv_buffer+4);
		pthread_mutex_lock(&hostq_lock);
		hostq[host_id].host_stat = host_status;
		pthread_mutex_unlock(&hostq_lock);
		}//if recv_size >0	

	}//while

}//void

#if 0
//this function is transferred into host_monitor.c
void host_monitor_transfer_0(){
}
#endif

#if 0
//depricated function to be (*removed*) in new version
void host_query(){
}//host_query
#endif

/*
 * function init the host ip list and host status list
 */
void hostq_init(){
	int i;
	for(i=0;i<MAX_NUM_HOSTS;i++){
	pcstat[i] = HOST_STAT_ONLINE;
	}//for
	for(i=1;i<=MAX_NUM_HOSTS;i++){
	hostq[i].host_id = i;
	hostq[i].host_stat = HOST_STAT_ONLINE;
	strncpy(hostq[i].ip,ips[i],strlen(ips[i]));
	//memncpy(hostq[i].ip,ips[i],strlen(ips[i]));
	}//for
}//void

void read_host_config_file(char *hostpath){

	//note there is no error in this function, it probably there exist memory leak within the function
	//hostpath = "hosts";

	int i;
        local_id  = -1;
        num_hosts = 0;        //NUM_HOSTS;
        int fd = open(hostpath,O_RDONLY,S_IRWXU);
	if(fd<0){
	fprintf(stderr, "error: %s open fd<0 no such host file:%s\n",__FUNCTION__,hostfile);
	exit(-1);
	}//if
        char rbuffer[1024];
        int nbytes = read(fd,rbuffer,1024);

        for(i=0;i<(MAX_NUM_HOSTS+1);i++){
                hosts[i] = (char *)malloc(sizeof(char)*64);
                ips[i]   = (char *)malloc(sizeof(char)*64);
        }//for
	hosts[0] = '\0';
	ips[0] = '\0';
        i = 1;
        char *p = rbuffer;

        while(nbytes >13){
        char *p1 = strstr(p,"<host>");
        if(p1==NULL) break;
        p1 = p1+6;
        char *p2 = strstr(p1,"</host>");
        if(p2==NULL) break;
        memcpy(hosts[i],p1,p2-p1);
	hosts[i][p2-p1] = '\0';
	memcpy(ips[i],p1,p2-p1);
	ips[i][p2-p1] = '\0';
        nbytes -= ((p2-p1)+13);
        p = p2+7;
        i++;
        }//nbytes

	num_hosts = i-1;
        struct hostent *host;
        struct in_addr *sin_addr;
        char *ip;

	#if 0
        for(i=1;i<=num_hosts;i++){
        host = gethostbyname(hosts[i]);
        sin_addr=((struct in_addr *)host->h_addr);
        ip = inet_ntoa(*sin_addr);
        }//for
	#endif

 	gethostname(hostname,64);
        host = gethostbyname(hostname);
        sin_addr=((struct in_addr *)host->h_addr);
        ip = inet_ntoa(*sin_addr);
	//fprintf(stderr,"hostname:%s num_hosts:%d ip:%s hosts[1]:%s\n",hostname,num_hosts,ip,hosts[1]);
	local_id == -1;
        for(i=1;i<=num_hosts;i++){
		int ip_str_len = strlen(ip);
              	if(memcmp(ip,(char *)(hosts[i]),ip_str_len)==0)
		{
		local_id = i;
		fprintf(stderr,"local ip:%s ips[i]:%s hosts[i]:%s local_id:%d\n",
							ip,ips[i], hosts[i], local_id);
		}//if
        }//for
	if(local_id == -1){
	fprintf(stderr,"error: ip:(%s) find in system /etc/hosts is not in %s\n",ip,hostpath);
	exit(-1);
	}//
}
