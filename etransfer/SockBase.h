#ifndef	__CSockBase_h
#define	__CSockBase_h
#include "NetAux.h"
/*#ifdef WIN32
//ÔÚsendº¯ÊýÖÐ
	#define MSG_NOSIGNAL 0
#define CLOSESOCKET(_Sock_) {::closesocket(_Sock_);_Sock_=-1;}
	#define EADDRINUSE WSAEADDRINUSE
	#define EINPROGRESS WSAEINPROGRESS
	#define ENOBUFS ERROR_INSUFFICIENT_BUFFER
	#define ENODATA ERROR_NO_MORE_ITEMS
	#if(_WIN32_WINNT >= 0x0500)
		#include <windows.h>
		#include <winsock2.h>
		#include <Ws2tcpip.h >
		#pragma comment(lib,"ws2_32.lib")
	#else
		#include <winsock.h>
		#pragma comment(lib,"wsock32.lib")
	#endif 
#else
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <strings.h>
	#include <unistd.h>
	#define MAX_PATH 255
#define CLOSESOCKET(_Sock_) {::close(_Sock_);_Sock_=-1;}
#endif
*/
#include "datatype.h"


class CSockBase
{
public:
	bool Init(short nPort);
	CSockBase();
	virtual ~CSockBase();
	int getSockFD(){return _sockFD;}
//	virtual int Bind(unsigned short nPort)=0;
	void detach(){_sockFD=-1;};
	int	getPort();
protected:
	int		_sockFD;
	unsigned short	_port;
	void	_Close();//
};

#endif // 
