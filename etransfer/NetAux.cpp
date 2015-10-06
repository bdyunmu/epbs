
#include "NetAux.h"
#include "SockTcp.h"
#include<algorithm>  
#include<functional>  
#include<ctype.h> 
//#include "global.h"
#include "IniFile.h"
//#include "UCUtility.h"
//#include "VoDInstance.h"

#ifdef WIN32
#include <iptypes.h>
#include <iphlpapi.h>
#include <ntddndis.h>
#pragma comment(lib,"Iphlpapi.lib")
#else
#include <net/if_arp.h>
#include <linux/hdreg.h>

#define MAXINTERFACES 16

#endif

bool CNetAux::getLocalIP(char* szLocalIP)
{
#ifdef WIN32

 	char szGateIP[MAX_PATH];
	memset(szGateIP, 0, MAX_PATH);

	szLocalIP[0] = 0;
	PIP_ADAPTER_INFO   pAdapterInfo;   
	PIP_ADAPTER_INFO   pAdapter   =   NULL;   
	DWORD   dwRetVal   =   0;   

	pAdapterInfo   =   (IP_ADAPTER_INFO   *)   malloc(   sizeof(IP_ADAPTER_INFO)   );   
	unsigned   long   ulOutBufLen   =   sizeof(IP_ADAPTER_INFO);   

	//   Make   an   initial   call   to   GetAdaptersInfo   to   get   
	//   the   necessary   size   into   the   ulOutBufLen   variable   
	if   (GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen)   ==   ERROR_BUFFER_OVERFLOW)     
	{   
		free(pAdapterInfo);   
		pAdapterInfo = (IP_ADAPTER_INFO   *)   malloc   (ulOutBufLen);     
	}   

	if   ((dwRetVal = GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen))   ==   NO_ERROR)     
	{   
		pAdapter = pAdapterInfo;   
		while(pAdapter)     
		{   
			memcpy(szGateIP, pAdapter->GatewayList.IpAddress.String, 16);
			if (strlen(szGateIP))
			{
				memcpy(szLocalIP, pAdapter->IpAddressList.IpAddress.String, 16);
				break;
			}


			pAdapter   =   pAdapter->Next;   
		}


	}

	free(pAdapterInfo);   
	return   true; 
#else
	return false;
#endif
};


//返回本地字节序
bool CNetAux::getLocalIP(uint32_t &ip){

#ifndef WIN32
	in_addr in;
	char buffer[8192];
	struct ifconf ifc;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1) return false;
	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_buf = buffer;
	if(ioctl(fd, SIOCGIFCONF, &ifc) == -1){
		close(fd);
		return false;
	}
	int count = ifc.ifc_len / sizeof(ifreq);
	for(int i = 0; i < count; ++i){
		if(ifc.ifc_req[i].ifr_addr.sa_family != AF_INET)
			continue;
		in = ((sockaddr_in&)ifc.ifc_req[i].ifr_addr).sin_addr;
		if((in.s_addr & 0x000000ff) != 0x7F){
			ip = in.s_addr;
			break;
		}
	}
	close(fd);
	ip = ntohl(ip);
#else // WIN32
/*
	SOCKET s = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	if(s == INVALID_SOCKET)
		return false;
	
	char outbuff[8192];
	DWORD outlen;
	DWORD ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, 0, 0, outbuff, sizeof(outbuff), &outlen, 0, 0);
	if(ret == SOCKET_ERROR)
		return false;
	
	INTERFACE_INFO* iflist = (INTERFACE_INFO*)outbuff;
	int ifcount = outlen / sizeof(INTERFACE_INFO);
	
	for(int i = 0; i < ifcount; ++i){
		in = ((sockaddr_in&)iflist[i].iiAddress).sin_addr;
		if((in.s_addr & 0x000000ff) != 0x7F){
			ip = in.s_addr;
			break;
		}
	}
	closesocket(s);
*/
	char szIp[MAX_PATH];
	if (getLocalIP(szIp))
	{
		CHost host;
		host.fromStr(szIp);
		ip = host.ip;
	}
#endif

	return true;
}

bool CNetAux::getLocalIPList(vector<uint32_t> &iplist)
{
	iplist.clear();

	in_addr in;
#ifndef WIN32
	char buffer[8192];
	struct ifconf ifc;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd == -1) return false;
	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_buf = buffer;
	if(ioctl(fd, SIOCGIFCONF, &ifc) == -1){
		close(fd);
		return false;
	}
	int count = ifc.ifc_len / sizeof(ifreq);
	for(int i = 0; i < count; ++i){
		if(ifc.ifc_req[i].ifr_addr.sa_family != AF_INET)
			continue;
		in = ((sockaddr_in&)ifc.ifc_req[i].ifr_addr).sin_addr;
		if((in.s_addr & 0x000000ff) != 0x7F){
			iplist.push_back(ntohl(in.s_addr));
		}
	}
	close(fd);
#else // WIN32
	
	SOCKET s = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	if(s == INVALID_SOCKET)
		return false;
	
	char outbuff[8192];
	DWORD outlen;
	DWORD ret = WSAIoctl(s, SIO_GET_INTERFACE_LIST, 0, 0, outbuff, sizeof(outbuff), &outlen, 0, 0);
	if(ret == SOCKET_ERROR)
		return false;
	
	INTERFACE_INFO* iflist = (INTERFACE_INFO*)outbuff;
	int ifcount = outlen / sizeof(INTERFACE_INFO);
	
	for(int i = 0; i < ifcount; ++i){
		in = ((sockaddr_in&)iflist[i].iiAddress).sin_addr;
		if((in.s_addr & 0x000000ff) != 0x7F){
			iplist.push_back(ntohl(in.s_addr));
		}
	}
	closesocket(s);
#endif
	return true;
}

//返回本地字节序
uint32_t CNetAux::resolveHostName(const char *hostName){
	uint32_t iStartTime = CUtility::getTickCount();
	unsigned long ip = inet_addr(hostName);
	if (ip == INADDR_NONE){
		ip = 0;
		hostent* he = gethostbyname(hostName);
		if (he && he->h_addr_list){
			memcpy(&ip, he->h_addr_list[0], he->h_length);
		}
	}
	uint32_t iEndTime = CUtility::getTickCount();

	if (iEndTime > iStartTime + 500)
	{
		//cerr << "CNetAux::resolveHostName, hostName[ "<< hostName << "], time[" << iEndTime - iStartTime << "]\n";
	}
	return ntohl(ip);	
} 

bool CNetAux::resolveHostName(const char *sHost, char *outIp){
	uint32_t ip = CNetAux::resolveHostName(sHost);
	in_addr in;
	in.s_addr = htonl(ip);
	strcpy(outIp,inet_ntoa(in));
	return true;
}

/*
bool CNetAux::getLocalIPList(vector<uint32_t> &iplist){
	char hostname[256];
	if(gethostname(hostname,256))
		return false;
	struct hostent  * he = gethostbyname(hostname);
	unsigned long ip;
	int i = 0;
	while (he->h_addr_list[i])
	{
		memcpy(&ip, he->h_addr_list[i], he->h_length);//注意ipv4，在ipv6中会有问题
		ip = ntohl(ip);
		iplist.push_back(ip);
		i++;
	}
	return true;
}
*/
int CNetAux::setSockNonBlocking(int sock){
#ifdef WIN32
	unsigned long flag=1;
	return ioctlsocket(sock, FIONBIO, &flag);
#else
	int fflags = fcntl(sock, F_GETFL, 0);
	fflags |= O_NONBLOCK;
	return fcntl(sock, F_SETFL, fflags);
#endif
}

bool CNetAux::InitNetwork(){
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;	
	wVersionRequested = MAKEWORD( 2, 2 );
	return WSAStartup( wVersionRequested, &wsaData ) == 0;
#else
	return true;
#endif
}

//判断是否为局域网IP
bool CNetAux::isLocalhost( uint32_t ip){
	return (ip >= 167772161UL && ip <= 184549375UL) ||
		(ip >= 2886729728UL && ip <= 2894331903UL) ||
		(ip >= 3232235520UL && ip <= 3232301055UL);
}


bool CNetAux::ParseURL(const string& url, string& hostName, unsigned short &uPort, string &remotepath){
	string tmp = url;

	string newurl = "";
	tmp.erase(tmp.begin(),tmp.begin()+tmp.find_first_not_of(' '));  
	tmp.erase(tmp.begin()+tmp.find_last_not_of(' ' )+1,tmp.end());  
	for (unsigned long i = 0; i < tmp.length(); i ++){
		if (tmp[i] == '\\')
			newurl  += '/';
		else if (tmp[i] == ' '){
			newurl += "%20";
		}
		else
			newurl.append(1, tmp[i]);
			//newurl.push_back(tmp[i]);
	}
	
	if(newurl.substr (0, 7) != "http://"){
		return false;
		//	newurl = "http://" + newurl;		 
	}
	
	int pos = newurl.find ('/', 8);
	
	if(pos!=-1){
		hostName = newurl.substr(7, pos - 7);
		remotepath = newurl.substr (pos);
	}else{
		remotepath="/";
		hostName = newurl.substr (7);
	}
	uPort = 80;
	if((pos = newurl.find(':', 8)) != -1){
		string port;
		pos++;
		int pos1 = newurl.find('/', pos);
		if(pos1 == -1)
			port = newurl.substr(pos);
		else
			port = newurl.substr(pos, pos1 - pos);
		port.erase(remove_if(port.begin(),port.end(),ptr_fun(::isspace)),port.end());
		uPort = atoi(port.c_str());
	}
	//如果hostname里有:
	if((pos = hostName.find(':')) != -1){
		hostName = hostName.substr(0, pos);
	}
	//if(remotepath.at(0) == '/')
	//	remotepath = remotepath.substr(1);
	return true;
}
/*
bool CNetAux::checkTimeOut(int s, bool r, bool w, int t)
{
	timeval timeout;
	fd_set read_fds;
	fd_set write_fds;
	//fd_set error_fds;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int _nTimeOut = 10000;

	//FD_ZERO (&error_fds);
	FD_ZERO (&write_fds);
	FD_ZERO (&read_fds);
	
	//FD_SET (_socket, &error_fds);
	if (w)
	{
		timeout.tv_sec = (long)(_nTimeOut /1000);
		timeout.tv_usec = (long)(_nTimeOut %1000);
		//write_fds= error_fds;
		FD_SET (s, &write_fds);
	}

	
	if (r)
	{
		timeout.tv_sec = (long)(_nTimeOut/1000);
		timeout.tv_usec = (long)(_nTimeOut%1000); 
		//read_fds = error_fds;
		FD_SET (s, &read_fds);
	}

	timeval *tp;
	if (timeout.tv_sec || timeout.tv_usec)
		tp = &timeout;
	else
		tp = NULL;

	//int ret = select (_socket + 1, &read_fds, &write_fds, &error_fds, tp);
	int ret = select (s + 1, &read_fds, &write_fds, NULL, tp);

	if (ret == 1)// && !FD_ISSET(_socket, &error_fds))
		return 0;
	else
		return SOCKET_ERROR;
}
*/

//todo pBuffer长度受限，是阻塞的，可以考虑用线程
bool CNetAux::HttpPost(const string& url, char* pBuffer, int len){
	string hostName, remotePath;
	unsigned short port;
	ParseURL(url, hostName, port, remotePath);
	CSockTcp tcp;
	tcp.Bind(0);
	if(port == 0)
		port = 80;
	if(!tcp.Connect(resolveHostName(hostName.c_str()), port))
		return false;
	
	int buf_len = MAX_PATH + len;
	char* buf = new char[buf_len];
	memset(buf, 0, buf_len);
	_snprintf(buf, MAX_PATH - 1,
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Mozilla/4.0\r\n"
		"Content-Length: %d\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: Keep-Alive\r\n"
		"Accept: */*\r\n\r\n",
		remotePath.c_str(),
		hostName.c_str(),
		len);
	buf_len = strlen(buf);
	memcpy(buf + buf_len, pBuffer, len);
	buf_len += len;
	
	int sentbytes = 0;
	int sock = tcp.getSockFD();
	while (buf_len)	{		
		int ret = send(sock, buf + sentbytes, buf_len, MSG_NOSIGNAL);
		if (ret<=0)
			break;
		buf_len -= ret;
		sentbytes += ret;
	}
	if (buf_len == 0){
		int ret = recv(sock, buf, MAX_PATH, MSG_NOSIGNAL);
	}
	if (buf)
		delete[] buf;
	return buf_len == 0;
}

//pOut的大小，应该足够放下http头和body
//返回失败时，pOut放失败原因
bool CNetAux::HttpGet(const string& url, char* pOut, int outLen, HTTP_HEAD_ITEM &head){
	string hostName, remotePath;
	unsigned short port;
	if(!ParseURL(url, hostName, port, remotePath)){
		strcpy(pOut, "ParseURL failed!");
		return false;
	}
	if(port == 0) port = 80;
	CSockTcp tcp;
	tcp.Bind(0);
	if(!tcp.Connect(resolveHostName(hostName.c_str()), port)){
		strcpy(pOut, "Connect err!");
		return false;
	}
	int buf_len = 4096;
	char buf[4096];
	memset(buf, 0, buf_len);
	_snprintf(buf, MAX_PATH - 1,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Mozilla/4.0\r\n"
		"Connection: Keep-Alive\r\n"
		"Accept: */*\r\n\r\n",
		remotePath.c_str(),
		hostName.c_str());
	buf_len = strlen(buf);	
	
	int sentbytes = 0;
	int sock = tcp.getSockFD();
//	memset(&head, 0, sizeof(HTTP_HEAD_ITEM));
	while (buf_len)	{		
		int ret = send(sock, (char*)&buf + sentbytes, buf_len, MSG_NOSIGNAL);
		if (ret<=0)
			break;
		buf_len -= ret;
		sentbytes += ret;
	}
	if (buf_len == 0){
		char *prcvBuf = pOut;
		memset(pOut, 0, outLen);
		int recved = recv(sock, prcvBuf, outLen-1, MSG_NOSIGNAL);
		if( recved == 0){
			strcpy(pOut, "first recv err!");
			return false;
		}
		prcvBuf[recved] = 0;
		if( !CUtility::parseHttpHead(pOut, head) || head.Response != 200){
			//只处理200，不处理其它的
			strcpy(pOut, "parseHttpHead err,or response not 200!");
			return false;
		};
		if(head.ContentLength + head.bodyPos >= outLen){
			strcpy(pOut, "output buf is small!");
			return false;
		}
		//接受没有收到的body
		while(recved < head.ContentLength + head.bodyPos){
			int ret = recv(sock, prcvBuf + recved, outLen-1-recved, MSG_NOSIGNAL);
			if(ret == 0)
				return recved == head.ContentLength + head.bodyPos;
			recved += ret;
		}
	}else{
		return false;
	}
	return true;
}


bool CNetAux::HttpHead(const string& url, HTTP_HEAD_ITEM &head){
	string hostName, remotePath;
	unsigned short port;
	if(!ParseURL(url, hostName, port, remotePath)){
		return false;
	}
	if(port == 0) port = 80;
	CSockTcp tcp;
	tcp.Bind(0);
	if(!tcp.Connect(resolveHostName(hostName.c_str()), port)){
		return false;
	}
	int buf_len = 4096;
	char buf[4096];
	memset(buf, 0, buf_len);
	_snprintf(buf, MAX_PATH - 1,
		"HEAD %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: Mozilla/4.0\r\n"
		"Connection: Close\r\n"
		"Accept: */*\r\n\r\n",
		remotePath.c_str(),
		hostName.c_str());
	int needSend_len = strlen(buf);	
	
	int sentbytes = 0;
	int sock = tcp.getSockFD();
//	memset(&head, 0, sizeof(HTTP_HEAD_ITEM));
	while (sentbytes<needSend_len)	{		
		int ret = send(sock, (char*)&buf + sentbytes, needSend_len - sentbytes, MSG_NOSIGNAL);
		if (ret<=0){
			return false;
		}
		sentbytes += ret;
	}
	
	char *prcvBuf = (char*)&buf;
	//memset(pOut, 0, outLen);
		//接受没有收到的body
	int recved	 = 0;
	bzero((char*)&head, sizeof(head));
	while(recved==0 || recved < head.ContentLength + head.bodyPos){
		int curRecved = recv(sock, prcvBuf + recved, buf_len-recved-1, MSG_NOSIGNAL);
		if( curRecved <= 0){
			//strcpy(pOut, "first recv err!");
			return false;
		}
		recved += curRecved;
		prcvBuf[recved] = 0;
		if( !CUtility::parseHttpHead(prcvBuf, head)){
			//只处理200，不处理其它的
			continue;
		};
		if(head.Response == 200) return true;
		if(head.Response == 301 || head.Response == 302){
			return HttpHead(head.Location, head);
		}
	}
	return true;
}

bool CNetAux::getGateIP(char* szGateIP)
{
#ifdef WIN32
	szGateIP[0] = 0;
	PIP_ADAPTER_INFO   pAdapterInfo;   
	PIP_ADAPTER_INFO   pAdapter   =   NULL;   
	DWORD   dwRetVal   =   0;   

	pAdapterInfo   =   (IP_ADAPTER_INFO   *)   malloc(   sizeof(IP_ADAPTER_INFO)   );   
	unsigned   long   ulOutBufLen   =   sizeof(IP_ADAPTER_INFO);   

	//   Make   an   initial   call   to   GetAdaptersInfo   to   get   
	//   the   necessary   size   into   the   ulOutBufLen   variable   
	if   (GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen)   ==   ERROR_BUFFER_OVERFLOW)     
	{   
		free(pAdapterInfo);   
		pAdapterInfo = (IP_ADAPTER_INFO   *)   malloc   (ulOutBufLen);     
	}   

	if((dwRetVal = GetAdaptersInfo(   pAdapterInfo,   &ulOutBufLen))   ==   NO_ERROR)     
	{   
		pAdapter = pAdapterInfo;   
		while(pAdapter)     
		{   
			memcpy(szGateIP, pAdapter->GatewayList.IpAddress.String, 16);
			if (strlen(szGateIP))
			{
				//ktlog->log(CLocalLog::T_GLOBAL, "\tIP   Address:   \t%s\n",   pAdapter->IpAddressList.IpAddress.String);

				break;
			}
	
			pAdapter   =   pAdapter->Next;   
		}	
	}

	free(pAdapterInfo);   

	return   true; 
#else
	return false;
#endif
}

bool CNetAux::getRandomMacAddress(char* szMacAddr)
{
	//string randstr = g_pInfo->pCustomIniFile->readStr("VoD","RMAC","");
	//if (randstr.length() != 16)
	//{
		string randstr = "";
		for (int i = 0; i < 16; i++)
		{
			randstr += CUtility::IntToString(CUtility::Random() % 10);
		}
		//g_pInfo->pCustomIniFile->write("VoD","RMAC",randstr);
	//}
	//return randstr;
	strcpy(szMacAddr, randstr.c_str());
	return true;
}

bool CNetAux::getRandomHardDiskSeries(char * szSeries)
{
	//string randstr = g_pInfo->pCustomIniFile->readStr("VoD","HDNO","");
	//if (randstr.length() == 16)
	//{
	//	return randstr;
	//}

	string randstr = "";
	for (int i = 0; i < 16; i++)
	{
		randstr += CUtility::IntToString(CUtility::Random() % 10);
	}
	//g_pInfo->pCustomIniFile->write("VoD","HDNO",randstr);
	strcpy(szSeries, randstr.c_str());
	return true;
}

bool CNetAux::getMacAddress(char* szMacAddr)
{
	if (CNetAux::getPhysicalMacAddress(szMacAddr) && strlen(szMacAddr) && !strstr(szMacAddr, "000000000000") && !strstr(szMacAddr, "FFFFFFFFFFFF"))
		return true;


	if (CNetAux::getLogicalMacAddress(szMacAddr) && strlen(szMacAddr) && !strstr(szMacAddr, "000000000000") && !strstr(szMacAddr, "FFFFFFFFFFFF"))
		return true;
	szMacAddr[0]=0;
	return false; //CNetAux::getRandomMacAddress(szMacAddr);
}
bool CNetAux::getLogicalMacAddress(char* szMacAddr)
{
#ifdef WIN32
	char szGateIP[MAX_PATH];
	memset(szGateIP, 0, MAX_PATH);

	PIP_ADAPTER_INFO   pAdapterInfo;   
	PIP_ADAPTER_INFO   pAdapter = NULL;   
	DWORD	dwRetVal   =  0;	

	pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
	unsigned   long   ulOutBufLen	=	sizeof(IP_ADAPTER_INFO);   

	//	 Make	an	 initial   call   to   GetAdaptersInfo	 to   get	
	//	 the   necessary   size   into	 the   ulOutBufLen	 variable	
	if(GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)	== ERROR_BUFFER_OVERFLOW)	 
	{	
		free(pAdapterInfo);   
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen);	   
	}	

	if((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)	 
	{	
		pAdapter = pAdapterInfo;   
		while(pAdapter) 	
		{	
			sprintf( szMacAddr,"%02X%02X%02X%02X%02X%02X", 
				pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2], 
				pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5] ); 

			if (strcmp(szMacAddr, "000000000000") != 0)
				break;
			pAdapter = pAdapter->Next;	 
		}
	}

	free(pAdapterInfo);   
	return true;
#else
	bool bret = false;
	int fd, interface;
	struct ifreq buf[MAXINTERFACES];
	struct ifconf ifc;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >=0)
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if(!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
		{
			interface = ifc.ifc_len/sizeof(struct ifreq);
			while(interface > 0)
			{
				interface--;

				int sock, nret;  
				struct ifreq ifr;  
				sock = socket(AF_INET, SOCK_STREAM, 0);  
				if(sock < 0)   
				{ 
					continue;
				}  
				memset(&ifr, 0, sizeof(ifr));  
				strcpy(ifr.ifr_name, buf[interface].ifr_name);  
				nret = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr));  
				if(nret == 0)   
				{  
					uint8_t mac[6];
					memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
					sprintf( szMacAddr,"%02X%02X%02X%02X%02X%02X", 
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
					close(sock);
					bret = true;
					if (!strstr(szMacAddr, "000000000000"))
						break;  
				}
			}   
		}
	}	
	close(fd);
	return bret;
	/*if(!bret || strlen(szMacAddr) == 0)
	{
		getRandomMacAddress(szMacAddr);
	}
*/
	return true;
#endif

}

#define OID_802_3_PERMANENT_ADDRESS 0x01010101
//#define IOCTL_NDIS_QUERY_GLOBAL_STATS 0x00170002


bool CNetAux::getPhysicalMacAddress(char* szMacAddr)
{
#ifdef WIN32
	return GetWireLanRealMACAddr(szMacAddr); //dinghui add
#else
	
	return false;
#endif
}

#ifdef WIN32
typedef BOOL (*GetDescription)(char* desp);
#endif

bool CNetAux::getHardDiskSeries(char *szSeries)
{
	szSeries[0] =0;
#ifdef WIN32
	bool bResult = false;
	static string hdstr = "";
	string ptselfcheckdll_path = CUtility::getExePath() + "SelfCheck.dll";

	try
	{
		HINSTANCE ptselfcheckdll = ::LoadLibrary(ptselfcheckdll_path.c_str());
		if (ptselfcheckdll)
		{
			GetDescription GetDesriptionFunc = (GetDescription)GetProcAddress(ptselfcheckdll, "GetDesription");
			if (GetDesriptionFunc)
			{
				char environment[4096];
				memset(environment, 0, 4096);
				GetDesriptionFunc(environment);
				hdstr = CUtility::getKeyValueStr(environment, "hd");
				if (!hdstr.empty() || hdstr.find("00000000") != string::npos)
					bResult = true;
			}
		}
	}catch(...)
	{

	}

	if (!bResult)
	{
		DWORD diskid;
		char volumeName[MAX_PATH];
		DWORD MaximumComponentLength;
		DWORD FileSystemFlags;
		char FileSystemNameBuffer[MAX_PATH];
		if (GetVolumeInformation("c:\\",
			volumeName,	MAX_PATH,
			&diskid,
			&MaximumComponentLength,
			&FileSystemFlags,
			FileSystemNameBuffer,MAX_PATH
			))
		{
			hdstr=CUtility::enBase64((char*)&diskid,sizeof(diskid));//获得硬盘号
			bResult = true;
		}
	}
	if(bResult)
		strcpy(szSeries, hdstr.c_str());
	return bResult;
	
#else
/*
	int fd;
	struct hd_driveid hdinfo;
	string stret = "";
	fd = open("/dev/hda", O_NONBLOCK);
	
	if(fd >=0)
	{
		if(ioctl(fd, HDIO_GET_IDENTITY, (unsigned char*)&hdinfo) >= 0)
		{
			stret = (char *)&hdinfo.serial_no;
 		}
	}
	
	close(fd);
	return stret;
*/
	return false;
#endif

}
//////////////////////////////////////////////////////////////////////////
#ifdef WIN32
#define OID_802_3_PERMANENT_ADDRESS 0x01010101
#define OID_802_3_CURRENT_ADDRESS 0x01010102
#define IOCTL_NDIS_QUERY_GLOBAL_STATS 0x00170002

#define NCF_VIRTUAL				0x1 //说明组件是个虚拟适配器
#define NCF_SOFTWARE_ENUMERATED 0x2 //说明组件是一个软件模拟的适配器
#define NCF_PHYSICAL			0x4 //说明组件是一个物理适配器
#define NCF_HIDDEN				0x8 //说明组件不显示用户接口
#define NCF_NO_SERVICE			0x10 //说明组件没有相关的服务(设备驱动程序)
#define NCF_NOT_USER_REMOVABLE	0x20 //说明不能被用户删除(例如，通过控制面板或设备管理器)
#define NCF_MULTIPORT_INSTANCED_ADAPTER  0x40 //说明组件有多个端口，每个端口作为单独的设备安装。每个端口有自己的hw_id(组件ID)并可被单独安装，这只适合于EISA适配器
#define NCF_HAS_UI				0x80 //说明组件支持用户接口(例如，Advanced Page或Customer Properties Sheet)
#define NCF_FILTER				0x400 //说明组件是一个过滤器

#define NUMBEROFSSIDS 10 

bool IfWirelessAdapter(HANDLE hAdapter) 
{ 
	DWORD dwBytes, dwOIDCode; 
	NDIS_802_11_BSSID_LIST *pList; 
	pList = (NDIS_802_11_BSSID_LIST *)VirtualAlloc(NULL, 
		sizeof(NDIS_802_11_BSSID_LIST) * NUMBEROFSSIDS, 
		MEM_RESERVE | 
		MEM_COMMIT, 
		PAGE_READWRITE); 
	if (!pList) 
		return false; 

	memset(pList, 0, sizeof(NDIS_802_11_BSSID_LIST) * NUMBEROFSSIDS); 
	dwOIDCode = OID_802_11_BSSID_LIST_SCAN; 
	DeviceIoControl(hAdapter, 
		IOCTL_NDIS_QUERY_GLOBAL_STATS, 
		&dwOIDCode, 
		sizeof(dwOIDCode), 
		NULL, 
		0, 
		&dwBytes, 
		NULL);
	Sleep(50);//这里的Sleep是否必需？
	memset(pList, 0, sizeof(NDIS_802_11_BSSID_LIST) * NUMBEROFSSIDS); 
	dwOIDCode = OID_802_11_BSSID_LIST; 
	if (!DeviceIoControl(hAdapter, 
		IOCTL_NDIS_QUERY_GLOBAL_STATS, 
		&dwOIDCode, 
		sizeof(dwOIDCode), 
		pList, 
		sizeof(NDIS_802_11_BSSID_LIST) * NUMBEROFSSIDS, 
		&dwBytes, 
		NULL)) 
		return false; 

	return true; 
};


//得到一个网卡的永久地址
int GetWireLanMAC(char *szAdapterName,char *szMacAddr)
{
	DWORD	dwOIDCode;
	BYTE	outBuf[256]= {0};
	DWORD	BytesReturned;
	HANDLE	hPhysicalDev;
	char	szDeviceName[MAX_PATH];

	szDeviceName[0] = '\0';
	strcat(szDeviceName,"//./");
	strcat(szDeviceName,szAdapterName);

	hPhysicalDev = CreateFile(szDeviceName,
		GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, 0, 0);
	
	if (hPhysicalDev == INVALID_HANDLE_VALUE)
		return 0;

	dwOIDCode = OID_802_3_PERMANENT_ADDRESS;

	if(DeviceIoControl(hPhysicalDev, IOCTL_NDIS_QUERY_GLOBAL_STATS, (LPVOID)&dwOIDCode,4,outBuf,256,&BytesReturned,NULL))
	{
		sprintf(szMacAddr,"%02X%02X%02X%02X%02X%02X",outBuf[0],outBuf[1],outBuf[2],outBuf[3],outBuf[4],outBuf[5]);
	}
	else
	{
		sprintf(szMacAddr,"%d", ::GetLastError());
		return 0;
	}
	
	if (IfWirelessAdapter(hPhysicalDev))
	{
		return 1;
	}

	return 2;
}

bool  CNetAux::GetWireLanRealMACAddr(char* szAdapterMACAddr)
{
	//首先使用GetAdaptersInfo获得当前的所有网卡信息
	PIP_ADAPTER_INFO pAdapterInfo;  
	PIP_ADAPTER_INFO pAdapter = NULL;  
	DWORD dwRetVal = 0; 
	DWORD dwOutBufLen;
	char szAdapterName[MAX_PATH]; //设备名

	char top_SubKey[] = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}";
	char sub_SubKey[MAX_PATH] = "";

	CHAR     achKey[MAX_PATH] = ""; 
	CHAR     achClass[MAX_PATH] = "";  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 

	FILETIME ftLastWriteTime;      // last write time 

	DWORD	i;
	DWORD	retCode; 	
	DWORD	cchValue = MAX_PATH; 
	HKEY	hKey,hSubKey;	

	DWORD	dwCharacteristics;
	DWORD	dwCharacteristicslength;
	char	czNetCfgInstanceId[MAX_PATH];
	DWORD	dwNetCfgInstanceId = MAX_PATH;

	char	szAdapterNameArray[MAX_PATH * 10];//假设最大10个网卡
	memset(szAdapterNameArray, 0, MAX_PATH * 10);
	uint		nAdapterCounter = 0;
	BOOL	bFindPhysical = FALSE;

	if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		top_SubKey,0, KEY_ALL_ACCESS, &hKey ) != ERROR_SUCCESS)
		return false;
	

	//遍历下面每个Key
	for (i = 0, retCode = ERROR_SUCCESS; 
		retCode == ERROR_SUCCESS && nAdapterCounter < 10; i++) 
	{ 
		cchValue = MAX_PATH;
		retCode = RegEnumKeyEx(hKey, 
			i, 
			achKey,  
			&cchValue, 
			NULL, 
			NULL, 
			NULL, 
			&ftLastWriteTime); 
		if (retCode == (DWORD)ERROR_SUCCESS) 
		{
			sub_SubKey[0] = '\0';
			strcat(sub_SubKey,top_SubKey);
			strcat(sub_SubKey,"\\");
			strcat(sub_SubKey,achKey);
			//打开这个键下的Characteristics
			if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				sub_SubKey,	0, KEY_QUERY_VALUE, &hSubKey ) != ERROR_SUCCESS)
				continue;
			dwCharacteristicslength = sizeof(DWORD);
			//获得NetCfgInstanceId

			if ( RegQueryValueEx( hSubKey, "Characteristics", NULL, NULL,
				(LPBYTE)&dwCharacteristics, &dwCharacteristicslength) == ERROR_SUCCESS)
			{
				//判断dwCharacteristics值
				if ((dwCharacteristics & NCF_PHYSICAL) == NCF_PHYSICAL)
				{
					//其可能为实际物理网卡
					//获得其NetCfgInstanceId
					if ( RegQueryValueEx( hSubKey, "NetCfgInstanceId", NULL, NULL,
						(LPBYTE)czNetCfgInstanceId, &dwNetCfgInstanceId) == ERROR_SUCCESS)
					{
						memcpy(szAdapterNameArray + nAdapterCounter*MAX_PATH,czNetCfgInstanceId, strlen(czNetCfgInstanceId));
						nAdapterCounter++;
					}					
				}
			}
			RegCloseKey( hSubKey );
		}
	}
	//nAdapterCounter为系统中真实网卡的个数
	//szAdapterName为文件的描述符
	pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));	
	dwOutBufLen = sizeof(IP_ADAPTER_INFO); 
	if(GetAdaptersInfo( pAdapterInfo, &dwOutBufLen )   ==   ERROR_BUFFER_OVERFLOW)   
	{  
		free(pAdapterInfo);  
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (dwOutBufLen);    
	}


	//Get The real value 
	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &dwOutBufLen )) ==   NO_ERROR)  
	{  
		pAdapter = pAdapterInfo;
		while (pAdapter && !bFindPhysical)
		{  
			//获得其设备名
			szAdapterName[0] = '\0';
			strcat(szAdapterName,pAdapter->AdapterName);
			//获知此设备是否为物理网卡
			//有线网卡的地址优先高于无线网卡
			//判断真实网卡地址中是否有此描述
			for (i = 0;i < nAdapterCounter;i++)
			{
				if (strncmp(szAdapterName,szAdapterNameArray + i*MAX_PATH,strlen(szAdapterName)) == 0)
				{
					//bFindPhysical = TRUE;
					//判断其是否为有线网卡
					int nRetCode = GetWireLanMAC(szAdapterName,szAdapterMACAddr);
					//VoDLOG((LOG_DEBUG, "GetWireLanMAC, %s, res %d", szAdapterMACAddr, nRetCode));
					if (nRetCode == 2)
					{
						bFindPhysical = true;
						break;
					}
				}
			}
			pAdapter = pAdapter->Next;
		}
	}  
	free(pAdapterInfo);
	if (szAdapterMACAddr)
		return true;
	else
		return false;
};
#endif




