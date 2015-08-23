
//client for easy pbs v0.2
//www.hbyunlin.com.cn
//7/31/3024

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

#include "libmsg.h"
#include "libhost.h"
#include "libjobq.h"
#include "libuser.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

extern int local_id;
extern int num_hosts;
extern char hostname[64];
extern char hostfile[64];

char *hosts[(MAX_NUM_HOSTS+1)];
char *ips[(MAX_NUM_HOSTS+1)];

int main(int argc, char **argv){

	int i = 0;
	char commstr[128];
	int num_req_nodes = 1;
	int qj_id = -1;
	int qh_id = -1;
	int job_id = -1;
	int prog_id = -1;
	sprintf(hostfile,"hosts");

	if(argc<3){
	fprintf(stderr,"usage1:%s -hf hostfile -cs commstr -nn numnodes\n",argv[0]);
	fprintf(stderr,"usage2:%s -qj [jd|al]\n",argv[0]);
	fprintf(stderr,"usage3:%s -qh [hd|al]\n",argv[0]);
	fprintf(stderr,"usage4:%s -jc [jd|al]\n",argv[0]);
	return 0;
	}//if argc<3

	for(i=1;i<argc;i++){
		if(strcmp(argv[i],"-hf")==0){
			sprintf(hostfile,(argv[i+1]));
			prog_id = 1;
			continue;
		}//if
		if(strcmp(argv[i],"-cs")==0){
			sprintf(commstr,(argv[i+1]));
			continue;
		}//if
		if(strcmp(argv[i],"-nn")==0){
			num_req_nodes = atoi(argv[i+1]);
			continue;
		}//if
		if(strcmp(argv[i],"-qj")==0){
			qj_id = atoi(argv[i+1]);
			prog_id = 2;
			continue;
		}//if
		if(strcmp(argv[i],"-qh")==0){
			qh_id = atoi(argv[i+1]);
			prog_id = 3;
			continue;	
		}//if
		if(strcmp(argv[i],"-jc")==0){
			job_id = atoi(argv[i+1]);
			prog_id = 4;
			continue;		
		}//if
	}//for

	switch(prog_id){
	case 1:
	if(strlen(commstr)==1 || strlen(hostfile)==1){
			fprintf(stderr,"usage:%s -hf hostfile -cs commstr -nn numnodes\n",argv[0]);
			return;
	}else{
	epbscli_init();
	client_single_job_submit(local_id,1,commstr,num_req_nodes);
	job_submit_exit();
	}
	break;

	case 2:
	epbscli_init();
	prog_2(qj_id);
	break;

	case 3:
	epbscli_init();
	prog_3(qh_id);
	break;

	case 4:
	epbscli_init();
	prog_4(job_id);
	break;

	}//switch

	return 0;
}

	void prog_2(int qj_id){

	fprintf(stderr,"\t client query job stat:%d\n",qj_id);
	int result = client_query_job_stat(qj_id);

	}//prog_2

	void prog_3(int qh_id){
	
	fprintf(stderr,"\t client query host stat:%d\n",qh_id);
	int result = client_query_host_stat(qh_id);
	
        }//prog_3
	
	void prog_4(int job_id){
	
	fprintf(stderr,"\t client job cancel:%d\n",job_id);
	client_cancel_job(job_id);
	
	}//prog_4;
	
int display_job_info(batch_job_info *pjob){

	int i;
	struct tm *tmt;
	fprintf(stderr,"ID\tJID\tSTAT\tNODES\tUNAME\tJNAME\tTIME0\n");
	for(i=0;i<MAX_JOB_QUEUE;i++){
		tmt = localtime(&(pjob[i].time1));
		fprintf(stderr,"%d\t%d\t%d\t%d\t%s\t%s\t%s\n",	
			i,pjob[i].job_id,pjob[i].job_stat,pjob[i].num_req_nodes,
			pjob[i].username,
			"null",
			asctime(tmt));
	}//for
	fprintf(stderr,"\n");
}

int display_host_info(epbs_host_info *hosts){
	int i;
	epbs_host_info *phost;
	fprintf(stderr,"ID\tHID\tSTAT\tIP\n");
	for(i=0;i<MAX_NUM_HOSTS;i++){
		phost = (epbs_host_info *)&(hosts[i]);
		fprintf(stderr,"%d\t%d\t%d\t%s\n",i,phost->host_id,phost->host_stat,phost->ip);
	}//for
}//int

int client_query_job_stat(int job_id){

	//fprintf(stderr,"client_query_job_stat:%d\n",job_id);

	int portnumber = UDP_JOB_STAT_SRV_PORT_NUMBER;
	char *send_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	int server_machine_id = 1;
	*((int *)send_buffer) = MSG_TYPE_JOB_QUERY;
	*((int *)send_buffer+1) = local_id;
	*((int *)send_buffer+2) = server_machine_id;
	*((int *)send_buffer+3) = job_id;
	int nbytes = udp_send(local_id,server_machine_id,send_buffer,
			MSG_HEAD_SIZE*sizeof(int),portnumber);
	if(nbytes == MSG_HEAD_SIZE*sizeof(int))
		fprintf(stderr,"client_query_job stat send %d bytes successfully\n",nbytes);

	if(job_id > -1){
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
	char *recv_buffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	udp_recv(local_id,server_machine_id,recv_buffer,MSG_HEAD_SIZE*sizeof(int),portnumber);
	int get_job_id = *((int *)recv_buffer+3);
	if(get_job_id!=job_id){
		fprintf(stderr,"error! get_job_id:%d!=job_id:%d\n",get_job_id,job_id);		
		return 0;
	}//if
	int get_job_stat = *(int *)((int *)recv_buffer+4);
	fprintf(stderr,"ID\tJID\tSTAT\n");
	fprintf(stderr,"0\t%d\t%d\n", get_job_id,get_job_stat);	
	fprintf(stderr,"\n");	
	}//if job_id > -1

	if(job_id == -1){
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
	char *recv_buffer2 = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE+sizeof(int)*MAX_JOB_QUEUE);
	udp_recv(local_id,server_machine_id,recv_buffer2,
			MSG_HEAD_SIZE*sizeof(int)+sizeof(int)*MAX_JOB_QUEUE,portnumber);
	int i;
	int jobstat[MAX_JOB_QUEUE];
	fprintf(stderr,"ID\tJID\tSTAT\n");

	for(i=0;i<MAX_JOB_QUEUE;i++){
		jobstat[i] = *((int *)recv_buffer2+MSG_HEAD_SIZE+i);
		fprintf(stderr,"%d\t%d\t%d\n",i,i,jobstat[i]);	
	}//for
	fprintf(stderr,"\n");

	}//if job_id == -1
	
	if(job_id == -2){
	portnumber = UDP_JOB_STAT_CLI_PORT_NUMBER;
	char *recv_buffer3 = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE
					+sizeof(batch_job_info)*MAX_JOB_QUEUE);

	int recv_buffer3_size = sizeof(int)*MSG_HEAD_SIZE+sizeof(batch_job_info)*MAX_JOB_QUEUE;

	fprintf(stderr,"debug job_id == -2 recv_buffer3_size:%d\n",recv_buffer3_size);

	udp_recv(local_id,server_machine_id,recv_buffer3,
		MSG_HEAD_SIZE*sizeof(int)+sizeof(batch_job_info)*MAX_JOB_QUEUE,portnumber);
	int i;
	batch_job_info jobinfo[MAX_JOB_QUEUE];

	for(i=0;i<MAX_JOB_QUEUE;i++){
	batch_job_info *pjob;
	pjob = (batch_job_info *)(recv_buffer3+sizeof(int)*MSG_HEAD_SIZE + i*sizeof(batch_job_info));
	jobinfo[i].job_id = pjob->job_id;
	jobinfo[i].job_stat = pjob->job_stat;
	sprintf(jobinfo[i].username,pjob->username,strlen(pjob->username));
	jobinfo[i].time0 = pjob->time0;
	jobinfo[i].time1 = pjob->time1;
	jobinfo[i].time2 = pjob->time2;
	jobinfo[i].time3 = pjob->time3;
	jobinfo[i].num_req_nodes = pjob->num_req_nodes;
	}//for

	display_job_info(jobinfo);
	
	}//if job_id == -2

	return 0;
}//

int client_query_host_stat(int machine_id){

	if(machine_id < 0)
	return;

  	struct hostent *host = gethostbyname(hosts[machine_id]);
        int remote_fd  = socket(AF_INET,SOCK_DGRAM,0);
        int portnumber = UDP_HOST_QUERY_SRV_PORT_NUMBER;
        struct sockaddr_in server_addr, client_addr;
	fprintf(stderr,"client query host stat host(%d) port(%d)\n",machine_id,portnumber);
	int i;
	int type = MSG_TYPE_HOST;
	int src  = local_id;
	int dist = machine_id;    	//the first host server
	int nbytes;
	int server_machine_id = 1;

	int send_buffer_len  = sizeof(int)*MSG_HEAD_SIZE;
	char *send_buffer  = (char *)malloc(send_buffer_len);
	
	memcpy(send_buffer, (char *)&type, sizeof(int));
	memcpy(send_buffer+1*sizeof(int),(char *)&src,sizeof(int));
	memcpy(send_buffer+2*sizeof(int),(char *)&dist,sizeof(int));
	memcpy(send_buffer+3*sizeof(int),(char *)&machine_id,sizeof(int));

	nbytes = udp_send(local_id, server_machine_id, send_buffer, 
				send_buffer_len, portnumber);
	if(nbytes != send_buffer_len){
		fprintf(stderr,"error! client query host stat nbytes!=udp_send_buffer_len\n");
		return -1;
	}//if

	if(machine_id >= 1){

	int recv_buffer_len  = MSG_HEAD_SIZE*sizeof(int)+sizeof(int)*MAX_NUM_HOSTS;
	char *recv_buffer  = (char *)malloc(recv_buffer_len);
	portnumber = UDP_HOST_QUERY_CLI_PORT_NUMBER;
	nbytes = udp_recv(local_id, server_machine_id, recv_buffer, 
			recv_buffer_len, portnumber);
		
	if(nbytes != recv_buffer_len){
		fprintf(stderr,"error! client query host stat nbytes!=udp_recv_buffer_len\n");
		return -1;
	}
		
	fprintf(stderr,"HID\t HSTAT\n");
	for(i=0; i<MAX_NUM_HOSTS; i++){
	int stat = *(int *)(recv_buffer+sizeof(int)*MSG_HEAD_SIZE+i*sizeof(int));
	fprintf(stderr,"%d\t%d\n",i,stat);
	}//for

	}// if machine_id >= 1;

	if(machine_id == 0){
	
	int recv_buffer_len = MSG_HEAD_SIZE*sizeof(int)+sizeof(epbs_host_info)*MAX_NUM_HOSTS;
	char *recv_buffer   = (char *)malloc(recv_buffer_len);
	portnumber = UDP_HOST_QUERY_CLI_PORT_NUMBER;
	nbytes = udp_recv(local_id,machine_id,recv_buffer,
				recv_buffer_len,portnumber);

	if(nbytes != recv_buffer_len){
		fprintf(stderr,"error! client query host stat nbytes!=udp_recv_buffer_len\n");
		return -1;
	}

	epbs_host_info *phost = NULL;
	epbs_host_info hostinfo[MAX_NUM_HOSTS];

	for(i=0;i<MAX_NUM_HOSTS;i++){
	phost = (epbs_host_info *)(recv_buffer+
			MSG_HEAD_SIZE*sizeof(int)+sizeof(epbs_host_info)*i);
	hostinfo[i].host_id = phost->host_id;
	hostinfo[i].host_stat = phost->host_stat;
	strncpy(hostinfo[i].ip,phost->ip,32);
	}//for
	display_host_info(hostinfo);

	}//if machine_id == 0;

	return 0;
}//void

void batch_job_submit(int local_id, int remote_id, char *commstr, int num_nodes){
	epbscli_init();
	client_single_job_submit(local_id,1,commstr,num_nodes);
        job_submit_exit();
}

void client_cancel_job(int job_id){

	int portnumber = UDP_JOB_MONITOR_SRV_PORT_NUMBER;
	int type = MSG_TYPE_JOB_CANCEL;
	int src = local_id;
	int server_machine_id = 1;
	int dist = server_machine_id;
	int nbytes;
	char *sendbuffer = (char *)malloc(sizeof(int)*MSG_HEAD_SIZE);
	*(int *)sendbuffer = type;
	*((int *)sendbuffer+1) = src;
	*((int *)sendbuffer+2) = dist;
	*((int *)sendbuffer+3) = job_id;
	nbytes = udp_send(src,dist,sendbuffer,sizeof(int)*MSG_HEAD_SIZE,portnumber);
	
}


void client_single_job_submit(int local_id, int remote_id, char *commstr, int num_req_nodes){

	int portnumber = UDP_JOB_QUE_SRV_PORT_NUMBER;
	char *username = "ut";//whoami();
	int username_len = strlen(username)+1;

	int type = MSG_TYPE_JOB_SUBMIT;
	int src  = local_id;
	int dist = remote_id;  //ANY remote pc
	int commpath_len = strlen(commstr)+1;
	int nbytes;
	int job_msg_len = commpath_len+username_len+MSG_HEAD_SIZE*sizeof(int);
	char *msgbuffer = (char *)malloc(job_msg_len);

	memcpy(msgbuffer, (char *)&type, sizeof(int));
	memcpy(msgbuffer+1*sizeof(int),(char *)&src,sizeof(int));	
	memcpy(msgbuffer+2*sizeof(int),(char *)&dist,sizeof(int));
	memcpy(msgbuffer+3*sizeof(int),(char *)&commpath_len,sizeof(int));
	memcpy(msgbuffer+4*sizeof(int),(char *)&num_req_nodes,sizeof(int));
	memcpy(msgbuffer+5*sizeof(int),(char *)&username_len,sizeof(int));

	memcpy(msgbuffer+MSG_HEAD_SIZE*sizeof(int),(char *)commstr,commpath_len);
	memcpy(msgbuffer+MSG_HEAD_SIZE*sizeof(int)+commpath_len,(char *)username,username_len);

	//msgbuffer[sizeof(int)*MSG_HEAD_SIZE+commpath_len-1] = '\0';
	fprintf(stderr,"client single job submit port:%d dist:%d pcom:%s user:%s whoamis:%s\n",
				portnumber,dist,commstr,username,whoami());
	nbytes = udp_send(src,dist,msgbuffer,job_msg_len,portnumber);

	if((nbytes == -1)){
		fprintf(stderr,"client single job sumbit error:%s\n",strerror(errno));
		exit(1);
	}//fi

	if (nbytes != job_msg_len){
	fprintf(stderr,"client single job submit error: nbytes!=job_msg_len\n");
	exit(1);
	}
	//todo need to adjust the recv portnumber before udp_send
	//for multiple users
	//need to verify the user_id and submit token;

	portnumber = UDP_JOB_QUE_CLI_PORT_NUMBER;
	char *recvbuffer = (char *)malloc(MSG_HEAD_SIZE*sizeof(int));
	nbytes = udp_recv(local_id,remote_id,recvbuffer,MSG_HEAD_SIZE*sizeof(int),portnumber);
	
	int new_job_id = *((int *)(recvbuffer)+4);

	fprintf(stderr,"\nJOB\tJID\tSTAT\n");
	fprintf(stderr,"%s\t%d\t%s\n",commstr,new_job_id,"submitted");

}//void


void job_submit_exit(){
	int i= 0;
	for(i=1;i<=num_hosts;i++){
	}//for
}//void

void epbscli_init()
{
	host_configure(hostfile);
    	if(num_hosts <1){
                fprintf(stderr,"error! num_hosts is less than one\n");
                exit(1);
        }//if
        if(local_id<1 || local_id>num_hosts){
                fprintf(stderr,"local machine is not setup in the host list\n");
                fprintf(stderr,"local_id:%d num_hosts:%d\n",local_id,num_hosts);
                exit(1);
        }
        struct hostent *host = gethostbyname(hosts[local_id]);
        if(host == NULL){
                fprintf(stderr,"host is not setup in the host list\n");
                exit(1);
        }//if
	fprintf(stderr,"epbscli init is done\n");
}