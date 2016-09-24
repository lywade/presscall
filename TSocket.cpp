#include "TSocket.h"


bool _isSocketAlive(int viSocket)
{
    int  iLen;
    char acBuff[10];
    
    if (viSocket <= 0)
        return false;

    /* save the flags */
    int flags;
    flags =  fcntl(viSocket, F_GETFL, 0);

    /*set the socket to NONBLOCK*/
    if (fcntl(viSocket, F_SETFL, flags | O_NONBLOCK) < 0)
    {       
       close(viSocket);
       return false;    
    }

    /*check if viSocket has been closed? */
    iLen = recv(viSocket, acBuff, 1, MSG_PEEK); 
    if (iLen == 0)
    {
        close(viSocket);
        return false;
    }
    else if (iLen > 0)
    {
        /* restore the flags of socket*/
        if (fcntl(viSocket, F_SETFL, flags) < 0)
        {
            close(viSocket);
            return false;
        }
        
        return true;    
    }
    else  //iLen < 0
    {
        if ((iLen == -1) && 
            ((errno == EAGAIN) || (errno == EINTR) || (errno == EWOULDBLOCK)))
        {
            /* restore the flags of  socket*/
            if (fcntl(viSocket, F_SETFL, flags) < 0)
            {
                close(viSocket);
                return false;
            }
            
            return true;
        }
        else
        {
            close(viSocket);
            return false;
        }
    }
}


//只做绑定,使得子进程继承此绑定
bool TcpSrvSocket::prepareServer (const char *ip, unsigned short port)
{
    if (isReady)
    {
        return true;
    }
    if (port > 65534)
    {
        return false;
    }
    closeServer();
    
    struct sockaddr_in server;
    
    if (ip == NULL || strlen(ip) == 0)
    {
        server.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {   
        server.sin_addr.s_addr = inet_addr(ip);
    }
    
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    m_iSocket = socket(AF_INET,SOCK_STREAM,0);
    
    if (m_iSocket < 0)
    {
        return false;
    }

    //设置地址重用
    int iSetResult;       
    int option_on = 1;
    iSetResult=setsockopt( m_iSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&option_on, sizeof(option_on) );
    if (iSetResult != 0)
    {
        closeServer();
        return false;
    }
    
    if (0>bind(m_iSocket,(struct sockaddr *)&server,sizeof(struct sockaddr_in)))
    {
        closeServer();	
        return false;
    }
    m_iSrvPort = port;	
    isReady = true;	
    return true;	
}

bool TcpSrvSocket::openServer (const char *ip, unsigned short port)
{   
    if (!isReady && !prepareServer (ip, port))
    {
        return false;
    }
    if (0>listen(m_iSocket,5))
    {
        closeServer();
        return false;
    }

    bOpen = true;
    return true;
}

//开始监听
bool TcpSrvSocket::openServer ()
{
    if (!isReady)
    {
        return false;
    }
	
    if (0>listen(m_iSocket,5))
    {
        closeServer();
        return false;
    }

    bOpen = true;
    return true;	
};

void TcpSrvSocket::closeServer()
{
    isReady = false;
    bOpen = false;	
    if (m_iSocket > 0)
    {
        close(m_iSocket);
        m_iSocket = -1;
        m_iSrvPort = -1;
    }
}

int TcpSrvSocket::acceptClient()
{
    if (m_iSocket < 0)
    {
        return -1;
    }
	
    //文件描述符阻塞方法
    // 设置阻塞
    int iFdFlagOld=fcntl(m_iSocket,F_GETFL);
    if (iFdFlagOld<0)
    {
        return -1;
    }

    int iFdFlag = iFdFlagOld;
    int iRet = fcntl(m_iSocket, F_SETFL,iFdFlag & ~O_NONBLOCK);
    if (iRet<0)
    {
        return -1;
    }
    
    int iAcceptResult = accept(m_iSocket, NULL, NULL);

    //恢复
    iRet = fcntl(m_iSocket,F_SETFL,iFdFlagOld);
    if (iRet<0)
    {
        return -1;
    }    

     if (iAcceptResult < 0)
    {
        return -1;         
    }

    return(iAcceptResult);
}

int TcpSrvSocket::getServicePort()
{
    if (true == bOpen)
    {
        return m_iSrvPort;
    }
    return -1;
}

bool TcpSrvSocket::isOpen()
{
    return bOpen;
}
int TcpSrvSocket::getSockFd()
{
    return m_iSocket;
}
//----------------------------------------------------------------------------
void TcpCltSocket::SetBindAddr(unsigned int unLocalAddr,unsigned short usLocalPort)
{
	m_unLocalAddr = unLocalAddr;
	m_usLocalPort = usLocalPort;
}

bool TcpCltSocket::ConnectServer(
		unsigned int unHost, unsigned short usPort,int iTimeOutUs/*=-1*/)
{
	strcpy(m_szErrMsg,"");
	if ((m_iSocket>=0) && ((int)usPort == m_iRemotePort) && ((int)unHost == m_iRemoteHost))
	{
		return true;
	}

	if(unHost==0 || usPort==0)
	{
		sprintf(m_szErrMsg,"bad host:%d or port:%d![%d]",unHost,usPort,__LINE__);
		return false;
	}
	
	DisconnectServer();

	m_iRemoteHost = unHost; 
	m_iRemotePort = usPort;
	
	//create
	if ((m_iSocket = socket(AF_INET,SOCK_STREAM,0))<0)
	{
		sprintf(m_szErrMsg,"create socket failed!ret=%d[%d]",m_iSocket,__LINE__);
		return false;
	}

	//non block	
	int iFlag = 0;
	if (iTimeOutUs >= 0)
	{
		iFlag = fcntl(m_iSocket, F_GETFL) | O_NONBLOCK;
		fcntl(m_iSocket, F_SETFL, iFlag);
	}

	//bind
	if(m_unLocalAddr || m_usLocalPort)
	{
		struct sockaddr_in bind_addr;
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_port = htons(m_usLocalPort);
		bind_addr.sin_addr.s_addr = m_unLocalAddr;
		
	    if (0>bind(m_iSocket,(struct sockaddr *)&bind_addr,sizeof(struct sockaddr_in)))
	    {
	        return false;
	    }	
	}
	
	//connect
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(usPort);
	server.sin_addr.s_addr = unHost;
	int iRet = connect(m_iSocket,(struct sockaddr*)&server,sizeof(struct sockaddr_in));
	if((iRet != 0)&& (errno != EWOULDBLOCK) && (errno != EINPROGRESS))
	{
		DisconnectServer();
		sprintf(m_szErrMsg,"connect  failed!ret=%d[%d]",iRet,__LINE__);
		return false;
	}     

	if (iTimeOutUs >= 0)
	{
		//select wait
		fd_set WriteSet;   
		FD_ZERO(&WriteSet);
		FD_SET(m_iSocket,&WriteSet);
		struct timeval tv;   
		tv.tv_sec = iTimeOutUs/1000000; 
		tv.tv_usec = iTimeOutUs%1000000;

		iRet = select(FD_SETSIZE,NULL,&WriteSet,NULL,&tv);
		if (iRet <= 0 || !FD_ISSET(m_iSocket, &WriteSet))
		{
			DisconnectServer();
			sprintf(m_szErrMsg,"connect select timeout!ret=%d[%d]",iRet,__LINE__);
			return false;
		}   

		//getscokopt check
		int iSockErr = 0;
		int iSockErrLen = sizeof(iSockErr);
		if (getsockopt(m_iSocket, SOL_SOCKET, SO_ERROR,
					(char*)&iSockErr, (socklen_t*)&iSockErrLen))
		{
			DisconnectServer();
			sprintf(m_szErrMsg,"connect getsockopt failed![%d]",__LINE__);
			return false;
		}

		if(iSockErr != 0)
		{
			DisconnectServer();
			sprintf(m_szErrMsg,"connect getsockopt failed!![%d]",__LINE__);
			return false;	
		}

		//block
		iFlag &= ~O_NONBLOCK;
		fcntl(m_iSocket, F_SETFL, iFlag);	
	}

	return true;	
}

void TcpCltSocket::DisconnectServer()
{
	if(m_iSocket>=0)
	{
		close(m_iSocket);
	}
	m_iSocket = -1;
}

bool TcpCltSocket::ReConnect()
{
	DisconnectServer();
	return ConnectServer(m_iRemoteHost,m_iRemotePort);
}

int TcpCltSocket::TcpWrite (void *pBuff, int iLen)
{
	strcpy(m_szErrMsg,"");
	if (pBuff == NULL||iLen <= 0)
	{
		sprintf(m_szErrMsg,"param error![%d]",__LINE__);
		return -1;
	}
	if (m_iSocket < 0)
	{
		sprintf(m_szErrMsg,"not connect yet![%d]",__LINE__);
		return -2;
	}	
	
	return write(m_iSocket,pBuff,iLen);
}

int TcpCltSocket::TcpRead (void *pBuff,int iLen,unsigned int unTimeWaitUs/*=10000000*/)
{
	strcpy(m_szErrMsg,"");
	
	if (pBuff == NULL||iLen <= 0)
	{
		sprintf(m_szErrMsg,"param error![%d]",__LINE__);
		return -1;
	}

	if (m_iSocket < 0)
	{
		sprintf(m_szErrMsg,"not connect yet![%d]",__LINE__);
		return -2;
	}	
	
	fd_set allSet;   
	FD_ZERO(&allSet);
	FD_SET(m_iSocket,&allSet);
	struct timeval tv;
	tv.tv_sec = unTimeWaitUs/1000000;
	tv.tv_usec = unTimeWaitUs%1000000;
	
	int iRet = select(FD_SETSIZE,&allSet,NULL,NULL,&tv);
	if (iRet <= 0 || !FD_ISSET(m_iSocket, &allSet))
	{
		sprintf(m_szErrMsg,"read time out!select ret=%d[%d]",iRet,__LINE__);
		return -3;
	}         

    	int iReadLen = read(m_iSocket,pBuff,iLen);
	if (iReadLen <= 0)
	{
		sprintf(m_szErrMsg,"link close by remote host!read ret=%d[%d]",iReadLen,__LINE__);
		DisconnectServer();
	}
	return iReadLen;
}

