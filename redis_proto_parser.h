#ifndef _REDIS_PROTO_PARSER_H_
#define _REDIS_PROTO_PARSER_H_

#include <vector>


/*
	201203
	wdd
*/



enum
{
	RED_REQ_MULTI_BULK,
	RED_REQ_MULTI_INLINE,

	RED_REPLY_STATUS,
	RED_REPLY_ERR_MSG,
	RED_REPLY_NUM,
	RED_REPLY_BULK,
	RED_REPLY_MULTI_BULK
};

struct redis_args
{
	int type;
	int nil;
	
	//pair first arg len, second arg data
	std::vector<std::pair<int, const char*> > args;

};


/*
	ret: 
	-1 err
	 0 need more bytes
	len success, total len
	
*/
extern int redis_cmd_parse(const char* buf, int len, redis_args* rargs);


/*
	ret: 
	-1 err
	 0 need more bytes
	len success, total len
	
*/
extern int redis_reply_parse(const char* buf, int len, redis_args* rargs, unsigned long long &serial_num);


#endif


