/*
 * =====================================================================================
 *
 *       Filename:  NetAux.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2008年07月14日 17时06分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dr. Fritz Mehner (mn), mehner@fh-swf.de
 *        Company:  FH S眉dwestfalen, Iserlohn
 *
 * =====================================================================================
 */
#ifndef __NETAUX_h
#define __NETAUX_h
#include <stdio.h>
#include <string>
using namespace std;
#ifdef WIN32
	#define MSG_NOSIGNAL 0
	#define CLOSESOCKET(_Sock_) {::closesocket(_Sock_);_Sock_=-1;}
	#define EADDRINUSE WSAEADDRINUSE
	#define EINPROGRESS WSAEINPROGRESS
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define ENOBUFS ERROR_INSUFFICIENT_BUFFER
	#define ENODATA ERROR_NO_MORE_ITEMS

#else
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <netdb.h>
//#include <sys/sockio.h>
	#include <net/if.h>
	#define MAX_PATH 255
	#define SOCKET_ERROR -1
#define CLOSESOCKET(_Sock_) {::close(_Sock_);_Sock_=-1;}
#endif
#include "datatype.h"
#include "Utility.h"
class CNetAux{
public:
	static bool InitNetwork();
	static bool getLocalIP(char* szLocalIP);
	static bool getLocalIP(uint32_t &ip);
	static uint32_t resolveHostName(const char *sHost);
	static bool resolveHostName(const char *sHost, char *outIp);
	static bool getLocalIPList(vector<uint32_t>&);
	static bool isLocalhost( uint32_t ip);
	static bool HttpPost(const string& url, char* pBuffer, int len);
	static bool HttpGet(const string& url, char* pOut, int len, HTTP_HEAD_ITEM &head);
	static bool HttpHead(const string& url, HTTP_HEAD_ITEM &head);
	static bool ParseURL(const string& url, string& hostName, unsigned short &uPort, string &remotepath);
	/**
	@return 0 正常；<0 异常
	*/
	static int setSockNonBlocking(int sock);

	static bool getGateIP(char* szGateIP);
	static bool getLogicalMacAddress(char* szMacAddr);
	static bool getRandomMacAddress(char* szMacAddr);
	static bool getRandomHardDiskSeries(char *szSeries);
	static bool getPhysicalMacAddress(char* szMacAddr);
	static bool getMacAddress(char * szMacAddr);
	static bool getHardDiskSeries(char *szSeries);
//	static bool checkTimeOut(int s, bool r, bool w, int t);

	//add dinghui
	//当返回值为true时:1.成功获得了网卡真实物理地址 2.优先获得了有线网卡的物理地址 3.在有线网卡不存在的时候,获得了无线等网卡的物理地址
	//当返回值为false时:获得网卡地址出错
	static bool GetWireLanRealMACAddr(char* szAdapterMACAddr);
};

class CSockAddr{
public:
	CSockAddr()
	{
		reset();
	}
	CSockAddr(const sockaddr_in& addr) :
	m_addr(addr)
	{
	}
	CSockAddr(unsigned long Ip, unsigned short port)
	{
		bzero(&m_addr,sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_addr.s_addr = htonl(Ip);
		m_addr.sin_port = htons(port);
	}
	/*
	CSockAddr(string& host,unsigned short port)
	{
		bzero(&m_addr,sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		if(host=="")
			m_addr.sin_addr.s_addr =INADDR_ANY;
		else
			m_addr.sin_addr.s_addr = htonl(CNetAux::resolveHostName(host.c_str()));
		m_addr.sin_port = htons(port);
	}*/

	CSockAddr(const string& host,unsigned short port)
	{
		memset(&m_addr,0,sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		if(host=="")
			m_addr.sin_addr.s_addr =INADDR_ANY;
		else if ( (m_addr.sin_addr.s_addr = inet_addr(host.c_str ())) == INADDR_NONE)
		{
			hostent* hp = gethostbyname(host.c_str ());
			if (hp == 0) 
				return;
			memcpy(&(m_addr.sin_addr.s_addr), hp->h_addr, hp->h_length);
		}
		m_addr.sin_port = htons(port);
	}
	void fromStr(const string &str, uint16_t p=0){
		bzero(&m_addr,sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		uint32_t ip;
		uint16_t port;
		if(str.length() == 0){
			ip = port = 0;
		}
		char name[128];
		strncpy(name,str.c_str(),127);
		name[127] = 0;
		port = p;
		char *pp = strstr(name,":");
		if (pp) {
			port = atoi(pp+1);
			pp[0] = 0;
		}
		m_addr.sin_addr.s_addr = htonl(CNetAux::resolveHostName(name));
		m_addr.sin_port = htons(port);
	}

	
	void reset()
	{
		bzero(&m_addr,sizeof(m_addr));
	}

	/**
	*	Get the ip string of the sockaddr
	*/
	char* GetIPStr() const
	{
		return inet_ntoa(m_addr.sin_addr);
	}

	string IPtoStr() const{
		return string(inet_ntoa(m_addr.sin_addr));
	}

	void fromIntIP(uint32_t iIP, uint16_t p){
		bzero(&m_addr,sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_addr.s_addr = htonl(iIP);
		m_addr.sin_port = htons(p);
	}

	string ToString() const{
		char result[MAX_PATH];
		bzero(result, MAX_PATH);
		sprintf(result, "%s:%d",inet_ntoa(m_addr.sin_addr), GetPort());
		return string(result);
	}

	unsigned long	GetIP()	const {return ntohl(m_addr.sin_addr.s_addr);}
	unsigned short 	GetPort()const {return ntohs(m_addr.sin_port);}

	sockaddr_in 	GetAddr() const {return m_addr;}
	void 			SetAddr(const sockaddr_in& addr){m_addr = addr;}

	bool operator==(const CSockAddr& cmper) const
	{
		return ((GetIP()==cmper.GetIP())&&(GetPort()==cmper.GetPort()));
	}

	bool operator!=(const CSockAddr& cmper) const
	{
		return ((GetIP()!=cmper.GetIP())||(GetPort()!=cmper.GetPort()));
	}

	bool operator < (const CSockAddr& cmper) const
	{
		if (GetIP() < cmper.GetIP())
			return true;
		else if (GetIP() > cmper.GetIP())
			return false;
		else if (GetPort() < cmper.GetPort())
			return true;
		else
			return false;
		//		return ((GetIP() < cmper.GetIP()) || (GetPort() < cmper.GetPort()));
	}

	void operator=(CSockAddr cmper)
	{
		m_addr = cmper.GetAddr();
	}

	bool isValid(){return GetIP() == 0 || GetIP() == 0xffffffff ? false:true;};

private:
	sockaddr_in m_addr;
};

//本地字节序
class CHost
{
/*	inline unsigned int ip3()
	{
		return (ip>>24);
	}
	inline unsigned int ip2()
	{
		return (ip>>16)&0xff;
	}
	inline unsigned int ip1()
	{
		return (ip>>8)&0xff;
	}
	inline unsigned int ip0()
	{
		return ip&0xff;
	}
*/
public:
	CHost():
		ip(0),
		port(0){}
	CHost(unsigned int i, unsigned short p){
		ip = i;
		port = p;
	}
	CHost(const string ip, unsigned short p){
		fromStr(ip.c_str(), p);
	}
	
/*	void init(){
		ip = 0;
		port = 0;
	}
*/

	bool	isMemberOf(CHost &);

	void operator = (const CHost& h){
		ip = h.ip;
		port = h.port;
	}

	bool operator == (const CHost &h)
	{
		return (h.ip == ip) && (h.port == port);
	}

	bool operator!=(const CHost &h)
	{
		return (h.ip != ip) || (h.port != port);
	}

	bool isSame(CHost &h){
		return (h.ip == ip) && (h.port == port);
	}

	bool isValid() const{return (ip != 0);	}

	string IPtoStr() const{
		in_addr in;
		in.s_addr = htonl(ip);
		return string(inet_ntoa(in));
	}

/*	void	IPtoStr(char *str)
	{
		sprintf(str,"%d.%d.%d.%d",(ip>>24)&0xff,(ip>>16)&0xff,(ip>>8)&0xff,(ip)&0xff);
	}*/
	string toStr() const
	{
		char str[MAX_PATH];
		in_addr in;
		in.s_addr = htonl(ip);
		sprintf(str, "%s:%d", inet_ntoa(in), port);
		return string(str);
	}

/*	void fromStrIP(const char *str, uint16_t p){
		unsigned int ipb[4];
		unsigned int ipp;
		if (strstr(str,":"))
		{
			if (sscanf(str,"%03d.%03d.%03d.%03d:%d",&ipb[0],&ipb[1],&ipb[2],&ipb[3],&ipp) == 5)
			{
				ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
				port = ipp;
			}else
			{
				ip = 0;
				port = 0;
			}
		}else{
			port = p;
			if (sscanf(str,"%03d.%03d.%03d.%03d",&ipb[0],&ipb[1],&ipb[2],&ipb[3]) == 4)
				ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
			else
				ip = 0;
		}	
	}
*/
	void fromIntIP(uint32_t iIP, uint16_t p){
		ip = iIP;
		port = p;
	}

	void fromStr(const string &str, uint16_t p=0){
		if(str.length() == 0){
			ip = port = 0;
		}
		char name[128];
		strncpy(name,str.c_str(),127);
		name[127] = 0;
		port = p;
		char *pp = strstr(name,":");
		if (pp) {
			port = atoi(pp+1);
			pp[0] = 0;
		}
		ip = CNetAux::resolveHostName(name);
	}
	uint32_t ip;
	uint16_t port;
};

#endif //__NETAUX_h


