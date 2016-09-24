#include "Base.h"


//--------------------------------------------------------------------------
/*
总时间轴长度iTimeAllSpanMs毫秒
检测粒度EachGridSpanMs毫秒
总请求容量MaxNumInAllSpan个
*/
CLoadGrid::CLoadGrid(int iTimeAllSpanMs, int iEachGridSpanMs, int iMaxNumInAllSpan/*=0*/)
{
    m_iTimeAllSpanMs = iTimeAllSpanMs;
    m_iEachGridSpanMs = iEachGridSpanMs;
    m_iMaxNumInAllSpan = iMaxNumInAllSpan;

    m_iNumInAllSpan = 0;
    m_iCurrGrid = 0;

    m_iAllGridCount = (int)(m_iTimeAllSpanMs / (float)m_iEachGridSpanMs + 0.9);
    m_iGridArray = new int[m_iAllGridCount];
    memset(m_iGridArray, 0, m_iAllGridCount * sizeof(int));

    gettimeofday(&m_tLastGridTime, NULL);
}

CLoadGrid::~CLoadGrid()
{
    delete []m_iGridArray;
}

/*
根据当前时间更新内部时间轴
*/
void CLoadGrid::UpdateLoad(timeval *ptCurrTime)
{
    if (m_iTimeAllSpanMs <= 0 || m_iEachGridSpanMs <= 0)
    {
        return;
    }

    timeval tCurrTime;

    if (ptCurrTime)
    {
        memcpy(&tCurrTime, ptCurrTime, sizeof(timeval));
    }
    else
    {
        gettimeofday(&tCurrTime, NULL);
    }

    //流逝的时间us
    int iTimeGoUs = (tCurrTime.tv_sec - m_tLastGridTime.tv_sec) * 1000000
                    + tCurrTime.tv_usec - m_tLastGridTime.tv_usec;

    //时间异常或流逝的时间超过总时间轴而过期
    if ((iTimeGoUs < 0) || (iTimeGoUs >= m_iTimeAllSpanMs * 1000))
    {
        //全部清理
        memset(m_iGridArray, 0, m_iAllGridCount * sizeof(int));
        m_iNumInAllSpan = 0;
        m_iCurrGrid = 0;
        //取传进来的时间，防止总是进入此处
        m_tLastGridTime = tCurrTime;
        return;
    }

    //超过一个时间片
    int iEachGridSpanUs = m_iEachGridSpanMs * 1000;

    if (iTimeGoUs > iEachGridSpanUs)
    {
        //流逝掉iGridGoNum个时间片段，首尾循环
        int iGridGoNum = iTimeGoUs / iEachGridSpanUs;

        for (int i = 0; i < iGridGoNum; i++)
        {
            m_iCurrGrid++;

            if (m_iCurrGrid == m_iAllGridCount)
            {
                m_iCurrGrid = 0;
            }

            m_iNumInAllSpan -= m_iGridArray[m_iCurrGrid];
            m_iGridArray[m_iCurrGrid] = 0;
        }

        //更新最后记录的时间
        int iTimeGoGrid = iGridGoNum * iEachGridSpanUs;
        int iSec = (iTimeGoGrid / 1000000);
        int iuSec = (iTimeGoGrid % 1000000);

        m_tLastGridTime.tv_sec += iSec;
        m_tLastGridTime.tv_usec += iuSec;

        if (m_tLastGridTime.tv_usec > 1000000)
        {
            m_tLastGridTime.tv_usec -= 1000000;
            m_tLastGridTime.tv_sec++;
        }
    }
    //未超过一个时间片，不需要更新
    else
    {
        ;
    }
}

int CLoadGrid::CheckLoad(timeval tCurrTime, bool bTest)
{
    if (m_iTimeAllSpanMs <= 0 || m_iEachGridSpanMs <= 0 || m_iMaxNumInAllSpan <= 0)
    {
        return LR_NORMAL;
    }

    //根据当前时间更新时间轴
    UpdateLoad(&tCurrTime);

    if (m_iNumInAllSpan < m_iMaxNumInAllSpan)
    {
        if (!bTest)
        {
            //累加本次
            m_iNumInAllSpan++;
            m_iGridArray[m_iCurrGrid]++;
        }

        return LR_NORMAL;
    }

    //到达临界
    return LR_FULL;
}

/*
获取当前时间轴数据，获取之前要根据当前时间更新，否则
得到的是最后时刻的情况
*/
void CLoadGrid::FetchLoad(int &iTimeAllSpanMs, int &iReqNum)
{
    UpdateLoad();
    iTimeAllSpanMs = m_iTimeAllSpanMs;
    iReqNum = m_iNumInAllSpan;
}
//---------------------------------------------------------------
