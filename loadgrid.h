#ifndef __LOADGRID_H__
#define __LOADGRID_H__

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <string>


using namespace std;

/*
    负载检查器,CLoadGrid(100000,50,99999) 即
    在时间轴上以50ms为每次前进的跨度，在100s的
    总长时间内限制包量9999个
*/
class CLoadGrid
{
public:
    enum LoadRet
    {
        LR_ERROR = -1,
        LR_NORMAL = 0,
        LR_FULL = 1
    };
    CLoadGrid(int iTimeAllSpan, int iEachGridSpan, int iMaxNumInAllSpan = 0);
    ~CLoadGrid();

    //用户每次调用，检查是否达到水位限制
    //return enum LoadRet{};
    int CheckLoad(timeval tCurrTime, bool bTest = false);

    //获取当前水位信息
    void FetchLoad(int &iTimeAllSpanMs, int &iReqNum);

private:
    void UpdateLoad(timeval *ptCurrTime = NULL);

    int m_iTimeAllSpanMs;
    int m_iEachGridSpanMs;
    int m_iMaxNumInAllSpan;

    int m_iAllGridCount;            //需要的总格子数
    int *m_iGridArray;          //时间轴
    timeval m_tLastGridTime;
    int m_iNumInAllSpan;

    //当前位
    int m_iCurrGrid;
};
