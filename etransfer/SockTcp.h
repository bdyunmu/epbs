#ifndef	__CSockTcp_h
#define	__CSockTcp_h

#include "SockBase.h"

class CSockTcp : public CSockBase
{
public:
	CSockTcp();
	virtual ~CSockTcp(){}
	bool Bind(unsigned short);
	bool Connect(const char *sHostName, unsigned short nPort);
	bool Connect(unsigned long l_ip, unsigned short nPort);//ÎªÍøÂç×Ö½ÚÐò
	int Receive(char *buf, int iLen);
	void detach(){_sockFD=-1;bConnected=false;};
	bool setNonBlocking();
	int SafeRecv(char* buf, int iLen);
	int SafeSend(char* buf, int iLen);
//	int setTimeout(int rsecs, int wsecs);
private:
//	int checkTimeout(bool r, bool w);
protected:
	long	destIP;
	unsigned short destPort;
	bool	bConnected;
//	int rsecs;
//	int wsecs;
};

#endif // 
