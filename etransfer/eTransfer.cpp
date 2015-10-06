#include <stdexcept>
#include <fcntl.h>

#include "Utility.h"
#include "exceptions.h"
#include "eTransfer.h"

using std::runtime_error;

int eTransfer::MY_TIMEVAL_TO_TIMESPEC(struct timeval *tv,struct timespec *ts) {
	(ts)->tv_sec = (tv)->tv_sec;
	(ts)->tv_nsec = (tv)->tv_usec * 1000;	
	return 0;
}//int

eTransfer* eTransfer::get_instance()
{
	return NULL;
}//eTransfer

eTransfer::eTransfer()
	:m_tcp_sock(-1),m_terminated(false)
{
	m_tcp_listener = NULL;
	pthread_mutex_init(&m_wait_lock, NULL);
	pthread_cond_init(&m_cond, NULL);
}//eTransfer
int eTransfer::destroy_socket(USocket *usocket)
{
	usocket->m_status = SOCK_CLOSED;
	add_destroy_socket(usocket);
	return 0;
}//int

int eTransfer::add_destroy_socket(USocket *usocket)
{
	AutoLock lock(m_main_mutex);
	m_destroy_list.push_back(usocket);
	return 0;
}

int eTransfer::do_destroy_sockets()
{
	if(m_destroy_list.empty())
		return 0;
	m_main_mutex.on();
	list<USocket*>::iterator sit = m_destroy_list.begin();
	while(sit != m_destroy_list.end())
	{
		USocket *usock = *sit;
		switch(usock->m_type)
		{
			case SOCK_TYPE_TCP:
			{
				m_tcp_map.erase(usock->m_sock);
				break;
			}
			default:
				break;
		}
		usock->close();
		sit++;
	}//while
	m_main_mutex.off();
	return 0;
}

//depricated function
int eTransfer::create_socket(USocket*&usocket,CSockAddr *addr, int type, void*param)
{
	//todo
	CUtility::Sleep(100);
	int err = 0;
	usocket = NULL;
	if(type == SOCK_TYPE_TCP)
	{
		UTcpSocket *socket = new UTcpSocket();
		//
		//socket->set_utransfer(this);
		if(socket->init(*addr)==0 &&socket->connect() == 0)
		{
			usocket = socket;
		}
		else
		{
			err = socket->get_reason();
			delete socket;
			return err;
		}
	}//if
}
int eTransfer::set_conn_set(fd_set&rset,fd_set&wset,fd_set&eset)
{
	if(m_tcp_sock >=0)
		FD_SET(m_tcp_sock,&rset);
	map<int,UTcpSocket*>::iterator it, end;
	it = m_tcp_map.begin();
	end = m_tcp_map.end();
	int max_conn = m_tcp_sock;
	UTcpSocket *socket;
	int sock;
	for(;it!=end;it++)
	{
		socket = it->second;
		sock = socket->m_sock;
		if(sock>max_conn)
			max_conn = sock;
		if(socket->m_bneedreadselect)
			FD_SET(sock,&rset);
		if(socket->m_bneedwriteselect)
			FD_SET(sock,&wset);
		if(socket->m_bneedreadselect || socket->m_bneedwriteselect)
			FD_SET(sock,&eset);
	}
	return max_conn;	
}//int

USocket * eTransfer::accept(USocket *usocket)
{
	USocket *acc_socket = NULL;
	if(usocket != NULL)
	{
		if(usocket == m_tcp_listener)
		{
			if(!m_tcp_accept_list.empty())
			{
				m_main_mutex.on();
				acc_socket = m_tcp_accept_list.front();
				acc_socket->m_status = SOCK_CONNECTED;
				m_tcp_accept_list.pop_front();
				m_main_mutex.off();
			}//if
		}//if
	}//if
	else
	{
		if(!m_tcp_accept_list.empty())
		{
			m_main_mutex.on();
			acc_socket = m_tcp_accept_list.front();
			m_tcp_accept_list.pop_front();
			m_main_mutex.off();
		}//if
	}//else
	if(acc_socket != NULL)
	{
		acc_socket->m_status = SOCK_CONNECTED;
	}//	
	return acc_socket;

}//USocket
int eTransfer::proc_tcp_listen_sock()
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	int new_sock = ::accept(m_tcp_sock,(struct sockaddr*)&addr,&addrlen);
	if(new_sock>=0)
	{
		UTcpSocket *usocket = new UTcpSocket();
		CSockAddr caddr(addr);
		usocket->bind(new_sock, caddr);
		m_main_mutex.on();
		m_tcp_map[new_sock] = usocket;
		m_tcp_accept_list.push_back(usocket);
		m_main_mutex.off();	
	}//if
	return -1;
}
int eTransfer::proc_tcp_socket(fd_set& rset, fd_set&wset, fd_set&eset, list<USocket*>&rlist,list<USocket*>&wlist)
{
	int sock;
	UTcpSocket *socket;
	map<int,UTcpSocket*>::iterator it,end;
	end = m_tcp_map.end();
	for(it = m_tcp_map.begin();it!=end;it++)
	{
		socket = it->second;
		sock = socket->get_sock();
		if(FD_ISSET(sock,&eset))
		{
			socket->m_status = SOCK_ERROR;
			socket->m_bneedreadselect = false;
			socket->m_bneedwriteselect = false;
			continue;
		}
		if(FD_ISSET(sock,&wset))
			socket->m_bneedwriteselect = false;
		if(FD_ISSET(sock,&rset))
			socket->m_bneedreadselect = false;
		switch(socket->get_status())
		{
			case SOCK_CONNECTING:
			if(FD_ISSET(sock,&wset))
			{
				socket->m_status = SOCK_CONNECTED;
			}
			break;
		}//switch
	}
	return 0;
}

int eTransfer::proc_rw_set(fd_set&rset,fd_set&wset,fd_set&eset,list<USocket *>&rlist,list<USocket *>&wlist)
{
	if(m_tcp_sock>=0&&FD_ISSET(m_tcp_sock,&rset))
		proc_tcp_listen_sock();
	proc_tcp_socket(rset,wset,eset,rlist,wlist);
	return 0;
}
void eTransfer::add_event(WEvent *e)
{
	AutoLock lock(m_event_list_mutex);
	m_event_list.push_back(e);
}//void
void eTransfer::walk_through()
{
	fd_set rset;
	fd_set wset;
	fd_set eset;
	int maxfd;
	struct timeval wait_time;
	int cnt = 0;
	list<USocket *> rlist;
	list<USocket *> wlist;
	//while(!m_terminated)
	do{
		if(m_tcp_sock<0)
		{
			CUtility::Sleep(20);
			continue;
		}//if
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&eset);
		maxfd = set_conn_set(rset,wset,eset);
		wait_time = m_base_wait_time;
		maxfd = ::select(maxfd+1,&rset,&wset,&eset,&wait_time);
		if(maxfd>0)
		{
			proc_rw_set(rset,wset,eset,rlist,wlist);
			rlist.clear();
			wlist.clear();
		}//if
		else if(maxfd == 0){
		}
		else
			continue;
	}while(false);
}
int eTransfer::select(list<upoll_t>&dst, const list<upoll_t>&src,struct timeval*wtm,WEvent *e )
{
	int sock_avail = 0;
	int wait_ret;
	dst.clear();
	list<upoll_t>::const_iterator it,end;
	if(has_available_sockets(dst,src)>0)
		return 1;
	
	for(it = src.begin();it!=src.end();it++)
	{
		if((*it).usock->getType() == SOCK_TYPE_TCP)
		{
			if((*it).events & UPOLL_READ_T)
				((UTcpSocket *)(*it).usock)->m_bneedreadselect = true;
			else
				((UTcpSocket *)(*it).usock)->m_bneedreadselect = false;
			if((*it).events & UPOLL_WRITE_T)
				((UTcpSocket *)(*it).usock)->m_bneedwriteselect = true;	
			else
				((UTcpSocket *)(*it).usock)->m_bneedwriteselect = false;

		}//if
	}
	struct timespec spec_tm;
	struct timeval cur_tm, result;
	gettimeofday(&cur_tm, NULL);
	timeradd(&cur_tm,wtm,&result);
	MY_TIMEVAL_TO_TIMESPEC(&result,&spec_tm);
	//pthread_mutex_lock(&m_wait_lock);
	wait_ret = 1;
	do
	{
		//fprintf(stderr,"(pthread_cond_timedwait)\n");
		CUtility::Sleep(50);
		//wait_ret=pthread_cond_timedwait(&m_cond,&m_wait_lock,&spec_tm);
	}
	while((sock_avail = has_available_sockets(dst,src))<=0 && (wait_ret--) > 0);
	//pthread_mutex_unlock(&m_wait_lock);
	//fprintf(stderr,"(after has_available_sockets) sock_avail:%d\n",sock_avail);
	return sock_avail;

}//int

int eTransfer::has_available_sockets(list<upoll_t>&dst, const list<upoll_t>&src)
{
	list<upoll_t>::const_iterator it, end;
	int count = 0;
	upoll_t up;
	for(it = src.begin(),end=src.end();it!=end;it++)
	{
		if(it->usock->is_error())
		{
			up.usock = it->usock;
			up.events = UPOLL_ERROR_T;
			up.pointer = it->pointer;
			dst.push_back(up);
			count++;
		}
		else
		{
			if(it->events&UPOLL_WRITE_T && it->usock->can_write())
			{
				up.usock = it->usock;
				up.events = UPOLL_WRITE_T;
				up.pointer = it->pointer;
				dst.push_back(up);
				count++;
			}
			if(it->events & UPOLL_READ_T)
			{
				bool can_read= false;
				if(it->usock == m_tcp_listener){
				can_read = !m_tcp_accept_list.empty();
				//fprintf(stderr,"(debug) (m_tcp_accept_list) (empty) (can_read):(%d)\n",(int)can_read);
				}//if
				else 
				can_read = it->usock->can_read();
				if(can_read)
				{
					up.usock = it->usock;
					up.events = UPOLL_READ_T;
					up.pointer = it->pointer;
					dst.push_back(up);
					count++;
				}//if
			}//if
		}//else
	}//for
	return count;
}
int eTransfer::init_tcp(int port)
{
	int ret;
	struct sockaddr_in addr;
	int sock_opt;
	if((m_tcp_sock = socket(PF_INET,SOCK_STREAM,0))<0)
		return CUtility::GetLastErr();
	
	sock_opt = 1;
	if(setsockopt(m_tcp_sock,SOL_SOCKET,SO_REUSEADDR,(char *)&sock_opt,sizeof(sock_opt))<0)
	ret = -1;	
	//goto _ERROR;
	if(CNetAux::setSockNonBlocking(m_tcp_sock)<0)
	ret = -1;	
	//goto _ERROR;
	sock_opt = 1024*256;
	if(setsockopt(m_tcp_sock,SOL_SOCKET,SO_RCVBUF,(char *)&sock_opt,sizeof(sock_opt))<0)
	ret = -1;	
	//goto _ERROR;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	int try_limit = 0;	
	for(try_limit = 5; try_limit>0;try_limit--)
	{
		addr.sin_port = htons(port);
		if(bind(m_tcp_sock,(sockaddr *)&addr,sizeof(addr))<0)
		{
			if(CUtility::GetLastErr() == EADDRINUSE)
				port = 10000+(CUtility::Random()%10000);
			else
				ret = -1;//goto _ERROR;
		}//if
		else
		{
			if(listen(m_tcp_sock,5)<0)
				ret = -1;//goto _ERROR;
			break;
		}//else
	}	
				
	if (try_limit == 0){
		CLOSESOCKET(m_tcp_sock);
		m_tcp_sock = -1;
		return -1;
	}//if
	m_tcp_port = port;
	m_tcp_listener = new UTcpSocket();
	return 0;
//_ERROR:
	ret = CUtility::GetLastErr();
	CLOSESOCKET(m_tcp_sock);
	m_tcp_sock = -1;
	return ret;
}//int

//Mutex::Mutex(){}
//Mutex::~Mutex(){}

