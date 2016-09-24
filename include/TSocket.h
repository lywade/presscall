/******************************************************************************

                  版权所有 (C), 2001-2011, 钟超宇

 ******************************************************************************
  文 件 名   : TSocket.h
  版 本 号   : V1.0
  作    者   : 钟超宇
  生成日期   : 2005年5月16日
  功能描述   : Socket服务端和客户端类定义
  依赖关系   : 独立可用
******************************************************************************/
#ifndef __TSOCKET_H__
#define __TSOCKET_H__

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <iostream>

using namespace std;

class TcpSrvSocket
{
private:
    int m_iSocket;	
    int m_iSrvPort;
    bool bOpen;
    bool isReady;	
public:
    TcpSrvSocket(){m_iSocket = -1;bOpen = false;m_iSrvPort = -1;isReady=false;};
    ~TcpSrvSocket(){closeServer();};
    bool prepareServer (const char *ip, unsigned short port);	
    //开启服务,ip=NULL表示绑定全部addr
    bool openServer ();
    bool openServer (const char *ip, unsigned short port);
    void closeServer ();
    int acceptClient(); 
    int getServicePort();
    int getSockFd();	
    bool isOpen();
};

class TcpCltSocket
{
private:
	int m_iSocket;	
	int m_iRemoteHost;
	int m_iRemotePort;

	char m_szErrMsg[256];

	unsigned int m_unLocalAddr;
	unsigned short m_usLocalPort;
public:
	TcpCltSocket()
	{
		m_iSocket = -1;
		m_iRemoteHost = 0;
		m_iRemotePort = 0;  
		memset(m_szErrMsg,0,sizeof(m_szErrMsg));

		m_unLocalAddr = 0;
		m_usLocalPort = 0;
	};
	~TcpCltSocket(){DisconnectServer();};
	
	bool ConnectServer(unsigned int unHost, unsigned short usPort,int iTimeOutUs=-1);	
	void DisconnectServer ();
	bool ReConnect();
	
	char* getErrMsg(){return m_szErrMsg;}
	int getRemoteAddr(){return m_iRemoteHost;}
	int getRemotePort(){return m_iRemotePort;}

	int TcpWrite (void *pBuff, int iLen);	
	int TcpRead (void *pBuff,int iLen,unsigned int unTimeWaitUs=10000000);

	int GetSocket(){return m_iSocket;}
	bool IsConnect(){return m_iSocket>=0?true:false;};
	void SetBindAddr(unsigned int unLocalAddr,unsigned short usLocalPort);
};

bool getip( const string &ifrname, string &ipv4 );
bool isInNet(const char *ip );
#endif

