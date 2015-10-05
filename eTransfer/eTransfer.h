#ifndef _ETRANSFER_H
#define _ETRANSFER_H

#include <map>
#include <list>
#include <time.h>

#include "usocket.h"
#include "utcp_socket.h"
//#include "uudp_socket.h"
//#include "rusocket.h"
//#include "rupacket.h"
#include "thread.h"

using std::map;
using std::list;

enum
{
	UPOLL_READ_T=1,
	UPOLL_WRITE_T=2,
	UPOLL_ERROR_T=4,
};

struct upoll_t
{
	USocket *usock;
	short events;
	void *pointer;
};

class eTransfer
{

public:
	static eTransfer* get_instance();
	int init_tcp(int port);
	//int init_udp(int port);
	UTcpSocket *get_tcp_listener()
	{
		return m_tcp_listener;
	}//UTcpSocket
	int select(list<upoll_t>&dst,const list<upoll_t>&src,struct timeval*tm,WEvent *e=NULL);
	inline int has_available_sockets(list<upoll_t>&dst,const list<upoll_t>&src);
	inline int set_conn_set(fd_set& rset,fd_set&wset,fd_set& eset);
	inline int MY_TIMEVAL_TO_TIMESPEC(struct timeval *, struct timespec *);
	USocket * accept(USocket * usocket);
	eTransfer();
	void walk_through();	
	int proc_tcp_listen_sock();
	int proc_tcp_socket(fd_set&rset,fd_set&wset,fd_set&eset,list<USocket*>&rlist,list<USocket*>&wlist);
	int proc_rw_set(fd_set&rset,fd_set&wset,fd_set&eset,list<USocket*>&rlist,list<USocket*>&wlist);
	void add_event(WEvent*e);	
	int create_socket(USocket*&usocket,CSockAddr *addr, int type, void*param);
private:
	pthread_cond_t m_cond;
	int m_tcp_sock;
	int m_tcp_port;
	UTcpSocket *m_tcp_listener;
	map<int,UTcpSocket*>m_tcp_map;
	list<UTcpSocket*>m_tcp_accept_list;
	pthread_mutex_t m_wait_lock;
	Mutex m_main_mutex;
	Mutex m_send_mutex;
	struct timeval m_base_wait_time;	
	bool m_terminated;				
	list<WEvent *> m_event_list;
	Mutex m_event_list_mutex;
};
#endif
