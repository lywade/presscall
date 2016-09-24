#ifndef _MYTIME_H_
#define _MYTIME_H_
#include <stdint.h>

//extern volatile uint32_t gnow;		//当前时间
//extern char gnowstr[30];	//当前时间的字符串

/*
 * 初始化时间相关环境
 */
//extern int init_time();
/*
 * 取当前时间（gnow会自动更新）
 * return		当前时间
 */
//extern uint32_t nowtime();
/*
 * 取当前时间的字符串表示
 * timestr		存储缓冲区指针
 */
extern const char* nowtimestr(char* timestr);
/*
 * 取某个特定时间的字符串表示
 * timestr		存储缓冲区指针
 */
extern const char* gettimestr(char* timestr, uint32_t tm);
/*
 * 取当前时间表示的tick数
 * return		tick数
 */
//extern uint64_t gettick();
/*
 * 转化秒数为tick数
 * sec			待转换的秒数
 * return		tick数
 */ 
//extern uint64_t maketick(double sec);
#endif

