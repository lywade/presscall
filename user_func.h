#ifndef _USERFUNC_H_
#define _USERFUNC_H_

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include "presscall.h"
#include <assert.h>
#include "cJSON.h"
#include "http.hpp"
#include <string>
#include <map>
#include "buffer.h"


using namespace std;

/*
用户实现DoNone接口即可
*/
class CUserFunc
{
public:
    CUserFunc(int socket_fd, struct TConfig g_Config);

    ~CUserFunc();

    void WriteSocket(struct TUsrResult *pResult);

    void ReadSocket(struct TUsrResult *pResult);

	void WriteCommandToBuf(struct TUsrResult *pResult);

    int PassportMsg(string cmd, string ticket, string project_code, string group, string type, int ars_num);

    int PhpPress(string arr);

    string randomgetstr(int len);
    string randomgetnumstr();
    int	getKey(int sequence, int type, string &key);
    void getValue(int length, string &value);
    void getField(int length, string &field);
    int genCommand(int type, int iVlen, int iFlen, int iFnum, string &fmt, string &name, char *wbuf, int &len);
    void genFormat(int type, int iFnum, char *wbuf, string &fmt, unsigned long long serial_num);
    string randomgetkey(int type);
    string randomgetfiled( );


private:
    pthread_mutex_t *m_pMutex;

    int iVlen, iFlen, iFnum;
    int iLev1, iLev2, iLev3;
    map<string, int> cmd_percent;
    
    struct timeval tBase;
    static __thread char *cmd_buf = NULL;

public:
    int m_sockfd;

    Buffer w_Buffer;
    Buffer r_Buffer;

    CLoadGrid *pLoadGrid = NULL;
};

#endif
