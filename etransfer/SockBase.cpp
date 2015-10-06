
#include "SockBase.h"
#include "Utility.h"
CSockBase::CSockBase() :
	_sockFD(-1)
{

}

CSockBase::~CSockBase()
{
	_Close();
}
void CSockBase::_Close(){
	if (_sockFD != -1){
		CLOSESOCKET(_sockFD);
		//close(_sockFD);
	}
	_sockFD=-1;
}
bool CSockBase::Init(short nPort)
{
	struct sockaddr_in addr1;
	int iLen;
    addr1.sin_family = AF_INET;
    addr1.sin_port = htons(nPort);
    addr1.sin_addr.s_addr = INADDR_ANY;
	bzero((char *)&(addr1.sin_zero), 8);
    if ((_sockFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		goto End;
	}
	iLen = 1;
	if (setsockopt(_sockFD, SOL_SOCKET, SO_REUSEADDR,(char *)&iLen,sizeof(iLen)) < 0){
		goto End;
	}
	if (setsockopt(_sockFD,IPPROTO_TCP,TCP_NODELAY ,(char *)&iLen, sizeof(iLen)) < 0){
		goto End;
	}
    if (bind(_sockFD, (struct sockaddr *)&addr1, sizeof(struct sockaddr)) < 0) {
		goto End;
	}
	return true;
End:
	return false;
}

int	CSockBase::getPort(){
	sockaddr_in addr1;
#ifdef WIN32
	int sin_size;
#else
	socklen_t sin_size;
#endif
	sin_size=sizeof(sockaddr_in);
	getsockname(_sockFD, (struct sockaddr*)&addr1, &sin_size);
	return ntohs(addr1.sin_port);	
}
