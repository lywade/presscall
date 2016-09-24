/*
* 文件: bossapi.h
* 功能: 定义了bossapi V4的日志接口
* 作者: taozeng
* 版本: 1.0
* 修改历史:
*       1.0 2012-07-24创建
*/

#ifndef WEBDEV_BOSS_API_V4_20120723_H
#define WEBDEV_BOSS_API_V4_20120723_H

#ifdef _WIN32
#define boss_vafun	 __cdecl
#else
#define boss_vafun
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define LOG_DEBUG 1
#define LOG_INFO 2 
#define LOG_WARN 3 
#define LOG_ERROR 4 
#define LOG_FATAL 5

/**********access log api****************************/
#define SEND_LOG_ACCESS(uin, module, oper, retcode, iflow, msginfo) do \
{ \
    sendAccessLog(uin, module, oper, retcode, iflow, msginfo, __FILE__, __LINE__, __FUNCTION__);\
} while (0);

/**********access log api****************************/
/**********error log api****************************/
// level: LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
#define SEND_LOG_ERROR(module, uin, cmd, level, errcode, msg) do \
{ \
        sendError(module, uin, cmd, level, errcode, msg, __FILE__, __LINE__, __FUNCTION__);\
} while (0);

int sendError(const char* module, long long uin, const char* cmd, 
              short level, int errcode, const char* msg, 
              const char* srcfile,  int srcline, const char* func);
/**********error log api****************************/


int sendAccessLog(long long uin, const char* module, const char* oper, 
		int retcode, long long iflow, const char* msg, 
        const char* srcfile,  int srcline, const char* func);


/********************************************************************
** 函数名: loginit
** 描述: 
**       初始化发送端口，非线程安全
** 参数:
**      _udp_port,  端口号，须和Boss Agent的监听端口号一致，为6578
** 返回值:
**      0		    成功
**	   其他 	    失败
*********************************************************************/
int loginit(const unsigned short _udp_port);

/********************************************************************
** 函数名: logprintf
** 描述: 
**       打一条日志到BOSS，非线程安全
** 参数:
**      _boss_id,   日志ID
**	   _format,     格式化日志内容
**      ...         不定参数
** 返回值:
**      0		成功
**      -1    申请内存失败
**      -2    _boss_id非法
**      -3    创建socket fd失败
**      -4    vsnprintf调用失败或者内容长度超出合法范围
**      -5    发送UDP失败
**	   其他 	未知错误
*********************************************************************/
int logprintf(const unsigned int _boss_id, const char* _format, ...);

/********************************************************************
** 函数名: sloginit
** 描述: 
**       初始化发送端口,线程安全
** 参数:
**      _udp_port,  端口号，须和Boss Agent的监听端口号一致，为6578
** 返回值:
**      0		    成功
**	   其他 	    失败
*********************************************************************/
int sloginit(const unsigned short _udp_port);

/********************************************************************
** 函数名: slogprintf
** 描述: 
**       打一条日志到BOSS，线程安全
** 参数:
**      _boss_id,   日志ID
**	   _format,     格式化日志内容
**      ...         不定参数
** 返回值:
**      0		成功
**      -1    申请内存失败
**      -2    _boss_id非法
**      -3    创建socket fd失败
**      -4    vsnprintf调用失败或者内容长度超出合法范围
**      -5    发送UDP失败
**	   其他 	未知错误
********************************************************************/
int slogprintf(const unsigned int _boss_id, const char* _format, ...);

/********************************************************************
** 函数名: convert_data
** 描述: 
**       转义字符内容，
        如果字符内容中可能含有NULL \r \n 逗号 斜杠 引号的
        必须首先调用本函数转义再调用(s)logprintf函数
** 参数:
**      _desc           存储转义完的数据的地址
**      _desc_capacity  存储转义完数据的容量
**      _src            需要转义的数据地址
**      _src_len        需要转义的数据长度
** 返回值:
**      0		    成功
**	   其他 	    失败
*********************************************************************/
int log_convert_data(char *_desc, const unsigned int _desc_capacity, 
		const char *_src, const unsigned int _src_len);

#ifdef __cplusplus
}
#endif
#endif

