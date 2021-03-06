
//host component for easy pbs v0.2
//www.bdyunmu.com
//9/30/2015

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

#include <list>
#include <signal.h>

#include "libmsg.h"
#include "libhost.h"
#include "libjobq.h"
#include "utransfer.h"
#include "PPTimers.h"

using namespace std;

extern int local_id;
int sockfd[MAX_NUM_HOSTS];
//int map_fd[MAX_NUM_HOSTS];
//char hostname[64];
extern int num_hosts;
int portnumber;

extern char *ips[MAX_NUM_HOSTS];
extern char *hosts[MAX_NUM_HOSTS];     //server   hosts

int pipes[2];
extern char hostfile[64];

void msg_configure(){
	num_hosts  = NUM_HOSTS;
	portnumber = MSG_PORT_NUMBER;
	sprintf(hostfile,"hosts");
	host_configure(hostfile);
}//void


int tcp_send1(char *r_ip, int port, char *buffer_, int len_, int portnumber_){

	map<USocket*, upoll_t> m_tcp_map;	
	CPPTimers timers(4);
	timers.setTimer(0,3000);
	timers.trigger(0);
	int totl_size = 0;
	char data[4096];
	UTransfer *transfer = UTransfer::get_instance();
	transfer->init_tcp(port);
	transfer->init_udp(port+1);
	list<upoll_t> src;
	list<upoll_t> dst;
	list<upoll_t>::iterator it;
	USocket *tcp_listener = transfer->get_tcp_listener();
	upoll_t ls_poll;
	ls_poll.events = UPOLL_READ_T;
	ls_poll.pointer = NULL;
	ls_poll.usock = tcp_listener;
	src.push_back(ls_poll);
	struct timeval base_tm = {0, 100};
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
		int res = transfer->select(dst, src, &wait_tm);
		if(res>0)
		{
			for(it=dst.begin();it!=dst.end();it++)
			{
			upoll_t up = *it;
			if(up.usock == tcp_listener)
			{
			if(up.events & UPOLL_READ_T)
			{
			USocket *sc = transfer->accept(tcp_listener);
			if(sc)
			{
			upoll_t new_up;
			//FILE *fp = fopen(
			new_up.pointer = NULL;
			new_up.usock = sc;
			new_up.events = UPOLL_WRITE_T;
			m_tcp_map[sc] = new_up;
			}//if
			}
			//TODO			
			}//if
			}//for

		}//if
	}//while
}


int tcp_send(char *srv_ip, int dist_, char *buffer_, int len_, int portnumber_){
	struct timeval wait_tm = {0, 0};	
	UTransfer *transfer = UTransfer::get_instance();
	transfer->init_tcp(4322);
	string host(srv_ip);
	int port_number = 0;
	char data[1024];
	CSockAddr raddr(host,portnumber_);
	USocket* tcp_sock1;

if(transfer->create_socket(tcp_sock1,&raddr,SOCK_TYPE_TCP) == 0){

	const char *fname = "tmp.result";
	
	list<upoll_t> src;
	list<upoll_t> obj;
	list<upoll_t>::iterator it;
	upoll_t pf;
		
	pf.events = UPOLL_WRITE_T;
	pf.pointer = NULL;
	pf.usock = tcp_sock1;
	src.push_back(pf);

	bool wait_creating = true;
	while(wait_creating)
	{
		obj.clear();
		int res = transfer->select(obj,src,&wait_tm);
	if(res>0)
	{
		it = obj.begin();
		if(it != obj.end())
		{
		if(it->events & UPOLL_ERROR_T)
		{
		cerr<<"can not connect to server"<<endl;
		return 0;
		}else if(it->events & UPOLL_WRITE_T)
		{

		cerr<<"connected to server."<<endl;
		wait_creating = false;
	
		}//else

		}//if
	}

	}//while

	src.clear();
	obj.clear();
	pf.events = UPOLL_READ_T;
	pf.pointer = NULL;	
	pf.usock = tcp_sock1;
	src.push_back(pf);
	
	bool bExit = false;
	struct timeval base_tm = {0, 100};

	while(!bExit){
	wait_tm = base_tm;
	int res = transfer->select(obj, src,&wait_tm);
	if(res>0)
	{
		it = obj.begin();
		if(it != obj.end())
		{
			pf = *it;
			if(pf.events & UPOLL_ERROR_T)
			{
			//cerr<<"connection error."<<endl;
			printf("connection error\n");
			bExit = true;
			}else if(pf.events & UPOLL_READ_T)
			{
			int remain_size = sizeof(data);
			int len = it->usock->recv(data,remain_size,NULL);
			if(len<=0)
			{
			bExit = true;
			break;
			}else
			{
			int filelen;
			//filelen += len;
			}//else
			}
		}
	}//if(res>0)

	}//while bExit


	}//if creat_socket1


}//int

int udp_recv(int src_, int dist_, char *buffer_, int len_, int portnumber_){

	int i;
        struct sockaddr_in server_addr;
        struct sockaddr_in client_addr;
        socklen_t sin_size = 0;
        int portnumber = portnumber_;
	char *udp_recv_buffer = buffer_;
	int udp_recv_buffer_len = len_;
        int recv_fd;

        if((recv_fd=socket(AF_INET,SOCK_DGRAM,0))==-1)
        {
        fprintf(stderr,"socket error:%s\n\a",strerror(errno));
        exit(1);
        }//if

        bzero(&server_addr,sizeof(struct sockaddr_in));
        server_addr.sin_family=AF_INET;
        server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
        server_addr.sin_port=htons(portnumber);

        if(bind(recv_fd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
        {
        fprintf(stderr,"udp recv bind error:%s port:%d\n\a", strerror(errno), portnumber_);
        exit(1);
        }//if

        sin_size=sizeof(struct sockaddr_in);

        //char buffer[MSG_BUFF_SIZE];
	char *buffer = (char *)malloc(udp_recv_buffer_len);
	//int rbytes;
	int rbytes = recvfrom(recv_fd, (char *)buffer,
		udp_recv_buffer_len, 0, (sockaddr *)&client_addr,&sin_size);
	//todo
	if(rbytes <=0 || rbytes > udp_recv_buffer_len){
		fprintf(stderr,"error udp_recv rbytes <=0 || rbytes>udp_recv_buffer_len\n");
		fprintf(stderr,"rbytes:%d udp_recv_buffer_len:%d\n",rbytes,udp_recv_buffer_len);
		exit(1);
	}//if

	int type = *((int *)buffer);
	int src = *((int *)buffer+1);
	int dist = *((int *)buffer+2);
	if(src != dist_){
		//fprintf(stderr,"warning src!=dist_ get_src:%d need_dist_:%d\n",src,dist_);
	}//if
	if(dist != src_){
		//fprintf(stderr,"warning dist!=src_ get_dist:%d need_src_:%d\n",dist,src_);
	}//if
	memcpy(udp_recv_buffer,buffer,rbytes);
	//fprintf(stderr,"udp recv %d bytes portnumber:%d\n",rbytes, portnumber);
	close(recv_fd);
	free(buffer);
	return rbytes;

}


int udp_send(int src_, int dist_, char *buffer_, int len_, int portnumber_){

	struct hostent *host = gethostbyname(ips[dist_]);
        int remote_fd  = socket(AF_INET,SOCK_DGRAM,0);
        int portnumber = portnumber_;
	char *udp_send_buffer = buffer_; 
	int udp_send_len = len_;
       	struct sockaddr_in server_addr;
	int nbytes = 0;
        bzero(&server_addr,sizeof(server_addr));
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(portnumber);
        server_addr.sin_addr=*((struct in_addr *)host->h_addr);

        if((nbytes = sendto(remote_fd, udp_send_buffer, udp_send_len,
                                        0,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)))==-1){
                fprintf(stderr,"udp send function error:%s\n",strerror(errno));
                exit(1);
        }//fi
        else if (nbytes == udp_send_len)
        	fprintf(stderr,"udp send %d bytes host:%s port:%d\n",
			nbytes, ips[dist_], portnumber_);
	close(remote_fd);
	return nbytes;
}//int



int msg_send(int src_, int dist_, char *buffer_, int len_){

	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server_addr;
	int portnumber = MSG_PORT_NUMBER;

        bzero(&server_addr,sizeof(server_addr));
        server_addr.sin_family=AF_INET;
        server_addr.sin_port=htons(portnumber);
        server_addr.sin_addr.s_addr=inet_addr(ips[dist_]);

	fprintf(stderr,"msg_send src:%d dist:%d len:%d ips[dist]:%s\n",	
			src_, dist_, len_, ips[dist_]);

        if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
        {
                fprintf(stderr,"msg_send connect error:%s\a\n",strerror(errno));
                exit(1);
        }//if
        else
		fprintf(stderr,"msg_send connect successful\n");

	char *msgbuffer = (char *)malloc(sizeof(char)*len_+sizeof(int)*MSG_HEAD_SIZE);
	char *pbuffer = (char *)((int *)msgbuffer+MSG_HEAD_SIZE);
	memcpy(pbuffer,buffer_,len_);
	*((int *)msgbuffer) = MSG_TYPE_DATA;
	*((int *)msgbuffer+1) = src_;
	*((int *)msgbuffer+2) = dist_;
	*((int *)msgbuffer+3) = len_;
	int wbytes = write(sockfd,msgbuffer,len_+sizeof(int)*MSG_HEAD_SIZE);
	fprintf(stderr,"msg_send send %d bytes to %s successful\n",wbytes,ips[dist_]);
	close(sockfd);

}//int


int msg_recv(int src, int dist, char *buffer, int len){

	if(src!=local_id){
		fprintf(stderr,"msg recv src!=local_id\n");
		exit(1);
	}//if

	int nbytes = len+sizeof(int)*MSG_HEAD_SIZE;
	int rbytes = len;
	char *msgbuffer = (char *)malloc(sizeof(char)*len+sizeof(int)*MSG_HEAD_SIZE);
	fprintf(stderr,"msg_recv(udp) start to read %d bytes src:%d dist:%d local(host:%s)\n",
							nbytes,src,dist,hosts[src]);
	
	rbytes = read(pipes[0],msgbuffer,nbytes);
	int m_type = *((int *)msgbuffer);
	int m_src  = *((int *)msgbuffer+1);
	int m_dist = *((int *)msgbuffer+2);
	int m_len  = *((int *)msgbuffer+3);

	fprintf(stderr, "m_type:%d m_src:%d src:%d m_dist:%d m_len:%d\n",
			m_type, m_src, src, m_dist, m_len);

	if(m_type != MSG_TYPE_DATA){
	perror("m_type!=MSG_TYPE_DATA");
	exit(1);
	}
	if(m_dist != src){
	perror("m_dist!=src");
	exit(1);
	}
	if(m_len != len){
	perror("m_len!=len");
	exit(1);
	}//if
	fprintf(stderr,"msg_recv get m_type:%d m_src:%d m_dist:%d m_len:%d\n",
				m_type,m_src,m_dist,m_len);
	char *pbuffer = (char *)((int *)msgbuffer+MSG_HEAD_SIZE);
	memcpy(buffer, pbuffer, len);
        return rbytes;

}

void msg_exit(){
	int i = 0;
	for (i = 1;i<=num_hosts;i++){
	//close(sockfd[i]);
	}//for
	fprintf(stderr,"msg_exit done\n");
}//void

void msg_check_configure(){

	if(num_hosts <1){
		fprintf(stderr,"check configure error: num_hosts is less than one\n");
		exit(-1);
	}//if

	if(local_id<1 || local_id>num_hosts){
		fprintf(stderr,"check configure error:local machine is not setup in the host list\n");
		fprintf(stderr,"local_id:%d num_hosts:%d\n",local_id,num_hosts);
		exit(-1);
	}

	struct hostent *host = gethostbyname(hosts[local_id]);
	if(host == NULL){
		fprintf(stderr,"check configure error:hostent get error\n");
		exit(-1);
	}//if

}//void


void msg_init()
{
	fprintf(stderr, "msg init\n");
	msg_configure();
	fprintf(stderr, "msg configure\n");
	msg_check_configure();
}//void
