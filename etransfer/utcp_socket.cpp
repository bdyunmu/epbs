/*
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
*/

//#pragma warning(disable:4786)

#include "utcp_socket.h"
#include "Utility.h"

UTcpSocket::UTcpSocket()
	:USocket(SOCK_TYPE_TCP)
{
	m_bneedreadselect = false;
	m_bneedwriteselect = false;
}

UTcpSocket::~UTcpSocket()
{
}

int
UTcpSocket::init(const CSockAddr& addr)
{
	m_addr = addr;

	return 0;
}

int
UTcpSocket::bind(int sock, const CSockAddr& remote_addr)
{
	m_sock = sock;
		
	if(CNetAux::setSockNonBlocking(m_sock) == 0){
		m_addr = remote_addr;
		return 0;
	}
		
	return CUtility::GetLastErr();
}

int
UTcpSocket::connect()
{
	m_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sock < 0)
		return CUtility::GetLastErr();

	struct sockaddr_in addr_in = m_addr.GetAddr();

	//if (fcntl(m_sock, F_SETFL, O_NONBLOCK) == 0)
	if(CNetAux::setSockNonBlocking(m_sock) == 0)
	{
		int ret = ::connect(m_sock, (sockaddr*)&addr_in, sizeof(struct sockaddr_in));
		if (ret == 0)
			m_status = SOCK_CONNECTED;
		else if (ret < 0 && 
			(CUtility::GetLastErr() == EINPROGRESS || CUtility::GetLastErr() == EWOULDBLOCK))
			m_status = SOCK_CONNECTING;
		else
			goto _ERROR;
		return 0;
	}

_ERROR:
	m_status = SOCK_ERROR;
	m_reason = CUtility::GetLastErr();
	CLOSESOCKET(m_sock);
	m_sock = -1;

	return m_reason;
}

ssize_t 
UTcpSocket::recv(char* buf, size_t len, CSockAddr* addr)
{
	if (m_status == SOCK_ERROR)
		return -1;

	ssize_t size;
	int nToRecv = CUtility::Min(len, MAX_RECV_LEN);

	if ((size = ::recv(m_sock, buf, nToRecv, MSG_NOSIGNAL)) < 0)
	{
		m_reason = CUtility::GetLastErr();
		//if (m_reason != EAGAIN)
		if (m_reason != EWOULDBLOCK)
			m_status = SOCK_ERROR;
	}
	else if (size == 0)
		m_status = SOCK_CLOSED;

	return size;
}

ssize_t 
UTcpSocket::send(const char *buf, size_t len, const CSockAddr* addr)
{
	if (m_status == SOCK_ERROR)
		return -1;

	int nToSend = CUtility::Min(len, MAX_SEND_LEN);
	ssize_t size = ::send(m_sock, buf, nToSend, MSG_NOSIGNAL);
	if (size < 0)
	{
		m_reason = CUtility::GetLastErr();
		if (m_reason != EWOULDBLOCK)
			m_status = SOCK_ERROR;
	}

	return size;
}

int
UTcpSocket::close()
{
	if (m_sock >= 0)
	{
		CLOSESOCKET(m_sock);
		m_status = SOCK_CLOSED;
		m_sock = -1;
	}

	return 0;
}

bool 
UTcpSocket::can_read()
{
	if (m_sock < 0)
		return false;

	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(m_sock, &rset);
	struct timeval wait_tm = {0, 0};
	int ret;

	if ((ret = select(m_sock + 1, &rset, NULL, NULL, &wait_tm)) > 0)
		return true;

	return false;
}

bool 
UTcpSocket::can_write()
{
	if (m_sock < 0)
		return false;

	fd_set wset;
	FD_ZERO(&wset);
	FD_SET(m_sock, &wset);
	struct timeval wait_tm = {0, 0};
	int ret;

	if ((ret = select(m_sock + 1, NULL, &wset, NULL, &wait_tm)) > 0)
		return true;

	return false;
}

int UTcpSocket::register_sock(int sock){
	if(sock <= 0)
		return -1;
	close();
	m_sock = sock;
	m_status = SOCK_CONNECTED;
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(m_sock, (sockaddr*)&addr, &len);
	//addr.
	m_addr.SetAddr(addr);
	return 0;
}

bool 
UTcpSocket::is_error()
{
	return m_status == SOCK_ERROR;
}


