#ifndef	__CSockTcpListenner_h
#define	__CSockTcpListenner_h
#include "SockTcp.h"

class CSockTcpListenner : public CSockTcp
{
public:
	bool Listen(){
		return listen(_sockFD, 5) == 0;
	}
/*	int Accept(int milliSeconds){
		struct timeval tv;
		tv.tv_sec = milliSeconds /1000;;
		tv.tv_usec = (milliSeconds %1000) * 1000;
		fd_set netFD;
		FD_ZERO (&netFD);
		FD_SET(_sockFD, &netFD);
		if(select(_sockFD + 1, &netFD, 0, 0, &tv) <= 0)
			return -1;
		sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		return accept(_sockFD, &addr, &addrlen);
	}
*/
	int Accept(){
		sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		return accept(_sockFD, &addr, &addrlen);
	};
	int Accept(sockaddr_in &addr){
		//sockaddr addr;
		bzero(&addr, sizeof(addr));
		socklen_t addrlen = sizeof(addr);
		return accept(_sockFD, (sockaddr*)&addr, &addrlen);
	};
	CSockTcpListenner(){};
	virtual ~CSockTcpListenner(){};
};

#endif // 
