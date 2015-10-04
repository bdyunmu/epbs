
//	head file for msg.h

#ifndef MSG_H
#define MSG_H

#define MSG_PORT_NUMBER 4010
//#define HOST_PORT_NUMBER 4020
//#define JOB_PORT_NUMBER 4030

#define UDP_MSG_SRV_PORT_NUMBER 4012
#define UDP_MSG_CLI_PORT_NUMBER 4014

//#define UDP_HOST_SRV_PORT_NUMBER 4022
//#define UDP_HOST_CLI_PORT_NUMBER 4024

#define UDP_HOST_MONITOR_SRV_PORT_NUMBER 4061
#define UDP_HOST_MONITOR_CLI_PORT_NUMBER 4028

#define UDP_HOST_STAT_SRV_PORT_NUMBER 4022
#define UDP_HOST_STAT_CLI_PORT_NUMBER 4024

#define UDP_CANCEL_JOB_SRV_PORT_NUMBER 4016
#define UDP_CANCEL_JOB_CLI_PORT_NUMBER 4018

#define UDP_INTER_JOB_SRV_PORT_NUMBER 4032
#define UDP_INTER_JOB_CLI_PORT_NUMBER 4034

#define UDP_JOB_RELT_SRV_PORT_NUMBER 4036
#define UDP_JOB_RELT_CLI_PORT_NUMBER 4038

#define UDP_JOB_QUE_SRV_PORT_NUMBER 4052
#define UDP_JOB_QUE_CLI_PORT_NUMBER 4054
#define UDP_JOB_STAT_SRV_PORT_NUMBER 4056
#define UDP_JOB_STAT_CLI_PORT_NUMBER 4058

#define UDP_JOB_MONITOR_SRV_PORT_NUMBER 4060
#define UDP_JOB_MONITOR_CLI_PORT_NUMBER 4062

#define MSG_BUFF_SIZE 2000
#define MSG_HEAD_SIZE 6

//1 TYPE
//2 SRC
//3 DIST
//4 M1
//5 M2

#define MSG_HOST_PATH_LEN 64

enum MSG_TYPE{
 	MSG_TYPE_DATA  	=1,
 	MSG_TYPE_HOST 	=2,
	MSG_TYPE_JOB_RUN    =3,
 	MSG_TYPE_JOB_SUBMIT =4,
 	MSG_TYPE_JOB_CANCEL =5,
 	MSG_TYPE_JOB_RETURN =6,
 	MSG_TYPE_JOB_QUERY  =7
};

void msg_init();
void msg_exit();
void msg_get_local_id();

//TODO
//int msg_send(int src, int dist, char *buffer, int len, int msg_id);
//int msg_recv(int src, int dist, char *buffer, int len, int msg_id);

void *msg_srv_process(void *);

int msg_send(int src, int dist, char *buffer, int len);
int msg_recv(int src, int dist, char *buffer, int len);
extern "C"{ 

int udp_recv(int src_, int dist_, char *buffer_, int len_, int portnumber_);
int udp_send(int src_, int dist_, char *buffer_, int len_, int portnumber_);
int tcp_send(int src_, int dist_, char *buffer_, int len_, int msg_type_);
int tcp_recv(int src_, int dist_, char *buffer_, int len_, int msg_type_);

}
#endif
