
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

#include "libhost.h"
#include "libmsg.h"

int local_id;
char hostname[64];
int num_hosts;

char hostfile[64];
char *ips[(MAX_NUM_HOSTS+1)];
char *hosts[(MAX_NUM_HOSTS+1)];     //server   hosts
int  pcstat[(MAX_NUM_HOSTS+1)];	

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
	if(dist_id>MAX_NUM_HOSTS || dist_id<=0) return NULL;
	else
	return ips[dist_id];
}//char
void epbs_host_status(){
	
	int portnumber = UDP_HOST_STAT_SRV_PORT_NUMBER;	
	fprintf(stderr,"host_status thread portnumber:%d\n",portnumber);
	int i;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int sin_size = 0;
	char recv_buffer[MSG_BUFF_SIZE];

	while(1){

	portnumber = UDP_HOST_STAT_SRV_PORT_NUMBER;
	int nbytes = udp_recv(local_id,0,recv_buffer,MSG_BUFF_SIZE,portnumber);
	int stat = 0;

	if(nbytes >0){

	int type  = *((int *)recv_buffer);
	int src   = *((int *)recv_buffer+1);
	int dist  = *((int *)recv_buffer+2);
	int host_id = *((int *)recv_buffer+3);
	int host_status = *((int *)recv_buffer+4);

	pthread_mutex_lock(&hostq_lock);
	hostq[host_id].host_stat = host_status;
	pthread_mutex_unlock(&hostq_lock);

	}//if nbytes >0	

	}//while

}//void

void host_query(){

	fprintf(stderr,"host query thread portnumber:%d\n",
			UDP_HOST_QUERY_SRV_PORT_NUMBER);
        int i;
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        int sin_size = 0;
        int portnumber = UDP_HOST_QUERY_SRV_PORT_NUMBER;
	char recv_buffer[MSG_BUFF_SIZE];
	char send_buffer[MSG_HEAD_SIZE*sizeof(int)+sizeof(int)*MAX_NUM_HOSTS];
       
	while(1){

	portnumber = UDP_HOST_QUERY_SRV_PORT_NUMBER;
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

	portnumber = UDP_HOST_QUERY_CLI_PORT_NUMBER;
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
	
	portnumber = UDP_HOST_QUERY_CLI_PORT_NUMBER;
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

void host_configure(char *hostpath){

	//note there is no error in this function, it probably there exist memory leak within the function
	//hostpath = "hosts";

	int i;
        local_id  = -1;
        num_hosts = 0;        //NUM_HOSTS;
        int fd = open(hostpath,O_RDONLY,S_IRWXU);
	if(fd<0){
	fprintf(stderr, "host configure error fd<0 no such host file:%s\n",hostfile);
	exit(1);
	}//if
        char buffer[256];
        int nbytes = read(fd,buffer,256);

        for(i=0;i<(MAX_NUM_HOSTS+1);i++){
                hosts[i] = (char *)malloc(sizeof(char)*64);
                ips[i]   = (char *)malloc(sizeof(char)*64);
        }//for

        i = 1;
        char *p = buffer;
        while(nbytes >10){

        char *p1 = strstr(p,"<host>");
        if(p1==NULL)
        break;

        p1 = p1+6;
        char *p2 = strstr(p1,"</host>");

        if(p2==NULL)
        break;

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
	fprintf(stderr,"error! ip:(%s) in /etc/hosts is not in /ebps/hosts\n",ip);
	exit(1);
	}//
}
