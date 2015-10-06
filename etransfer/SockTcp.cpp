
#include "SockTcp.h"
#include "Utility.h"

CSockTcp::CSockTcp(){//bConnected=false;
		bConnected=false;
	}
bool CSockTcp::Bind(unsigned short nPort){
	if(_sockFD != -1) //已经帮定了
		return false;
	if((_sockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return false;
	struct sockaddr_in sock_addr;
    sock_addr.sin_addr.s_addr = INADDR_ANY; 
	sock_addr.sin_family = AF_INET; 
	bzero((char *)&(sock_addr.sin_zero), 8);
	sock_addr.sin_port = htons(nPort); 
	int iLen = 1;
	if (setsockopt(_sockFD, SOL_SOCKET, SO_REUSEADDR,(char *)&iLen,sizeof(iLen)) < 0){
		goto err;
	}
	if (setsockopt(_sockFD,IPPROTO_TCP,TCP_NODELAY ,(char *)&iLen, sizeof(iLen)) < 0){
		goto err;
	}
	if (setsockopt(_sockFD,SOL_SOCKET,SO_KEEPALIVE ,(char *)&iLen, sizeof(iLen)) < 0){
		goto err;
	}
	if (bind(_sockFD, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr)) == -1)
		goto err;
	_port = nPort;
	return true;
err:
	 _Close();
	 return false;
}

bool CSockTcp::Connect(const char *sHostName, unsigned short nPort){
	unsigned long l_ip;
	struct sockaddr_in struc_addr;
	if(bConnected)
		return false;//already connected
	if(_sockFD < 0) 
		return false;
	//octPort=htons(nPort);
	if((l_ip=inet_addr(sHostName))==0)
		return false;
	struc_addr.sin_family =AF_INET;
	struc_addr.sin_port =htons(nPort);
	struc_addr.sin_addr.s_addr  = l_ip;
	bzero((char *)&(struc_addr.sin_zero), 8);
	if (connect(_sockFD, (struct sockaddr *)&struc_addr,sizeof(struct sockaddr)) == -1)
		goto err;
	bConnected = true;
	destIP = l_ip;
	destPort = nPort;
	return true;
err:
	l_ip=CUtility::GetLastErr();
	return false;
}

bool CSockTcp::Connect(unsigned long l_ip, unsigned short nPort){
	struct sockaddr_in struc_addr;
	if(bConnected)
		return false;//already connected
	if(_sockFD < 0) return false;
	struc_addr.sin_family =AF_INET;
	struc_addr.sin_port = htons(nPort);
	struc_addr.sin_addr.s_addr  = htonl(l_ip);
	bzero((char *)&(struc_addr.sin_zero), 8);
	if (connect(_sockFD, (struct sockaddr *)&struc_addr,sizeof(struct sockaddr)) == -1)
		goto err;
	bConnected = true;
	destIP = l_ip;
	destPort = nPort;
	return true;
err:
	return false;
}

int CSockTcp::SafeRecv(char* buf, int iLen)
{
	int nRecved = 0;
	int ret = 0;
	while(nRecved < iLen)
	{
		ret = Receive(buf + nRecved, iLen - nRecved);
		if (ret <= 0)
			break;

		nRecved += ret;
	}

	return nRecved;
}

int CSockTcp::SafeSend(char* buf, int iLen)
{
	if(!bConnected)
		return -1;

	int nSent = 0;
	int ret = 0;
	while(nSent < iLen)
	{
		int nToSend = CUtility::Min(iLen - nSent, MAX_SEND_LEN);
		ret = send(_sockFD, buf + nSent , nToSend, MSG_NOSIGNAL);

		if (ret <= 0)
		{
			break;
		}

		nSent += ret;
	}

	return nSent;

}

int CSockTcp::Receive(char *buf, int iLen){
	if(!bConnected) 
		return -1;

	int nToRecv = CUtility::Min(iLen, MAX_RECV_LEN);
	return recv(_sockFD, buf, nToRecv, MSG_NOSIGNAL);
}

bool CSockTcp::setNonBlocking(){
	if(_sockFD <= 0) return false;
	return CNetAux::setSockNonBlocking(_sockFD) == 0;
}



