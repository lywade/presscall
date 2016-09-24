/*
	201203
	wdd
*/


#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>

#include "redis_proto_parser.h"


using namespace std;


#define MAX_NUM_BNR 10
#define MAX_ARG_LEN 512*1024*1024


enum
{
	REQ_MULTI_ARG_NR,
	REQ_MULTI_ARG_LEN,
	REQ_MULTI_ARG_CONTENT,
	REQ_MULTI_DOLLAR,
	REQ_MULTI_R,
	REQ_MULTI_N,

	REQ_INLINE_SPACE,
	REQ_INLINE_CHAR,
	REQ_INLINE_N,

	REP_LINE_MSG,
	REP_LINE_N,

	REP_BULK_ARG_LEN,
	REP_BULK_ARG_CONTENT,
	REP_BULK_R,
	REP_BULK_N,

	REP_SESS_DIGITAL,
	REP_SESS_SEP,
	REP_SESS_R,
	REP_SESS_N
};

static inline int multi_bulk_parse(const char* buf, int len, redis_args* rargs)
{
	char num[MAX_NUM_BNR+1];
	int i, state, pos, argnr, arglen, st;

	/*multi bulk request*/
	i = 0;
	argnr = 1;
	state = REQ_MULTI_ARG_NR;

	for(pos=1; (pos < len) && argnr; pos++)
	{
		switch(state)
		{
			case REQ_MULTI_ARG_NR: 

				switch(buf[pos])
				{
					case '0' ... '9':
					case '-':
						if(i == MAX_NUM_BNR)
						{
							return -1;
						}
						num[i++] = buf[pos];
						break;
					
					case '\r':

						if(!i)
						{
							return -1;
						}

						num[i] = 0;							
						argnr = atoi(num);
						if(((argnr < 0) && (argnr != -1)) ||
							((num[0] == '-') && (argnr != -1)))
						{
							return -1;
						}

						if(argnr > 0)
						{
							++argnr;
						}
						else
						{
							/*empty or nil*/
							if((argnr == -1) && rargs)
							{
								rargs->nil = 1;
							}

							argnr = 1;
						}
						
						i = 0;
						st = REQ_MULTI_DOLLAR;
						state = REQ_MULTI_N;
						break;
					default:
						return -1;
				}
				
				break;

			case REQ_MULTI_ARG_LEN:

				switch(buf[pos])
				{
					case '0' ... '9':
					case '-':
						if(i == MAX_NUM_BNR)
						{
							return -1;
						}
						num[i++] = buf[pos];
						break;						
					case '\r':

						if(!i)
						{
							return -1;
						}

						num[i] = 0;
						arglen = atoi(num);
						if((arglen > MAX_ARG_LEN) 
							|| ((arglen < 0) && (arglen != -1)) 
							|| ((num[0] == '-') && (arglen != -1)))
						{
							return -1;
						}

						if(arglen >= 0)
						{
							st = REQ_MULTI_ARG_CONTENT;
						}
						else
						{
							/*nil*/
							st = REQ_MULTI_DOLLAR;

							if(rargs)
							{
								rargs->args.push_back(make_pair(-1, (const char*)0));
							}
						}

						i = 0;
						state = REQ_MULTI_N;
						break;
					default:
						return -1;
				}

				break;

			case REQ_MULTI_ARG_CONTENT:

				/*skip content bytes*/
				pos += arglen-1;
				if(pos >= len-1)
				{
					return 0;
				}

				/*got one arg*/
				if(rargs)
				{
					rargs->args.push_back(make_pair(arglen, buf+pos-arglen+1));
				}

				st = REQ_MULTI_DOLLAR;
				state = REQ_MULTI_R;

				break;
				
			case REQ_MULTI_R:

				if(buf[pos] != '\r')
				{
					return -1;
				}
				
				state = REQ_MULTI_N;
				break;

			case REQ_MULTI_N:

				if(buf[pos] != '\n')
				{
					return -1;
				}

				if(st == REQ_MULTI_DOLLAR)
				{
					--argnr;
				}
				
				state = st;
				break;

			case REQ_MULTI_DOLLAR:

				if(buf[pos] != '$')
				{
					return -1;
				}
				
				state = REQ_MULTI_ARG_LEN;
				break;

			default:
				/*impossible*/
				return -1;
		}
	}

	if((!argnr) && (state == REQ_MULTI_DOLLAR))
	{
		/*parse done*/
		return pos;
	}

	return 0;
}

static inline int inline_parse(const char* buf, int len, redis_args* rargs)
{
	int state, pos, begin, fin;

	/*inline request*/
	switch(buf[0])
	{
		case ' ':
		case '\t':
			begin = -1;
			state = REQ_INLINE_SPACE;
			break;

		case '\r':
			state = REQ_INLINE_N;			
			break;

		default:
			begin = 0;
			state = REQ_INLINE_CHAR;
			break;
	}

	for(fin=0,pos=1; (pos < len) && (!fin); pos++)
	{
		switch(state)
		{
			case REQ_INLINE_SPACE:

				switch(buf[pos])
				{
					case ' ':
					case '\t':
						break;

					case '\r':
						state = REQ_INLINE_N;
						break;

					default:
						begin = pos;
						state = REQ_INLINE_CHAR;
						break;
				}
				
				break;

			case REQ_INLINE_CHAR:

				switch(buf[pos])
				{
					case ' ':
					case '\t':
						
						if(rargs)
						{
							rargs->args.push_back(make_pair(pos-begin, buf+begin));
						}

						state = REQ_INLINE_SPACE;
						break;

					case '\r':

						if(rargs)
						{
							rargs->args.push_back(make_pair(pos-begin, buf+begin));
						}
						
						state = REQ_INLINE_N;
						break;

					default:
						break;
				}

				break;

			case REQ_INLINE_N:

				if(buf[pos] != '\n')
				{
					return -1;
				}

				fin = 1;
				
				break;
				
			default:
				/*impossible*/
				return -1;
		}
	}

	if(fin)
	{
		return pos;
	}

	return 0;
}

static inline int line_msg_parse(const char* buf, int len, redis_args* rargs)
{
	int state, pos, fin;

	state = REP_LINE_MSG;
	
	for(fin=0,pos=1; (pos < len) && (!fin); pos++)
	{
		switch(state)
		{
			case REP_LINE_MSG:

				if(buf[pos] == '\r')
				{
					if(rargs)
					{
						rargs->args.push_back(make_pair(pos-1, buf+1));
					}

					state = REP_LINE_N;
				}
				
				break;

			case REP_LINE_N:

				if(buf[pos] != '\n')
				{
					return -1;
				}

				fin = 1;

				break;
				
			default:
				break;
		}
	}

	if(fin)
	{
		return pos;
	}

	return 0;

}

static inline int bulk_parse(const char* buf, int len, redis_args* rargs)
{
	char num[MAX_NUM_BNR+1];
	int i, state, pos, arglen, fin, st;

	/*bulk reply*/
	i = 0;
	state = REP_BULK_ARG_LEN;

	for(fin=0,pos=1; (pos < len) && (!fin); pos++)
	{
		switch(state)
		{
			case REP_BULK_ARG_LEN:

				switch(buf[pos])
				{
					case '0' ... '9':
					case '-':
						if(i == MAX_NUM_BNR)
						{
							return -1;
						}
						num[i++] = buf[pos];
						break;						
					case '\r':

						if(!i)
						{
							return -1;
						}

						num[i] = 0;
						arglen = atoi(num);
						if((arglen > MAX_ARG_LEN) 
							|| ((arglen < 0) && (arglen != -1)) 
							|| ((num[0] == '-') && (arglen != -1)))
						{
							return -1;
						}

						if(arglen >= 0)
						{
							st = REP_BULK_ARG_CONTENT;
						}
						else
						{
							/*nil*/
							if(rargs)
							{
								rargs->args.push_back(make_pair(-1, (const char*)0));
							}

							st = REP_BULK_N;
						}

						state = REP_BULK_N;
						break;
					default:
						return -1;
				}

				break;

			case REP_BULK_ARG_CONTENT:

				/*skip content bytes*/
				pos += arglen-1;
				if(pos >= len-1)
				{
					return 0;
				}

				/*got one arg*/
				if(rargs)
				{
					rargs->args.push_back(make_pair(arglen, buf+pos-arglen+1));
				}

				st = REP_BULK_N;
				state = REP_BULK_R;

				break;
				
			case REP_BULK_R:

				if(buf[pos] != '\r')
				{
					return -1;
				}
				
				state = REP_BULK_N;
				break;

			case REP_BULK_N:

				if(buf[pos] != '\n')
				{
					return -1;
				}

				if(st == state)
				{
					fin = 1;
				}
				else
				{
					state = st;
				}

				break;

			default:
				/*impossible*/
				return -1;
		}
	}

	if(fin)
	{
		/*parse done*/
		return pos;
	}

	return 0;
}


/*
	ret: 
	-1 err
	 0 need more bytes
	len success, total len
	
*/
int redis_cmd_parse(const char* buf, int len, redis_args* rargs)
{
	int i, st, ret;

	if((!buf) || (len <= 0))
	{
		return -1;
	}

	i = -1;

	if(buf[0] == '@')
	{
		/*
			ext sess ctx
			such as
			@1234
		*/
		st = REP_SESS_DIGITAL;
		for(i = 1; (i<len) && (st!=REP_SESS_N); i++)
		{
			switch(st)
			{
			case REP_SESS_DIGITAL:

				switch(buf[i])
				{
					case '0' ... '9':
					case 'a' ... 'z':
					case '|':
					case '_':
						break;
		/*			case '|':
						if(i == 1)
						{
							return -1;
						}
						st = REP_SESS_SEP;
						break;*/
					case '\r':
						if(i == 1)
						{
							return -1;
						}
						st = REP_SESS_R;
						break;
					default:
						return -1;
				}

				break;
			/*case REP_SESS_SEP:

				switch(buf[i])
				{
					case '0' ... '9':
						st = REP_SESS_DIGITAL;
						break;
					default:
						return -1;
				}

				break;*/
			case REP_SESS_R:
				if(buf[i] != '\n')
				{
					return -1;
				}
				st = REP_SESS_N;
				break;
			}
		}

		if(st != REP_SESS_N)
		{
			return 0;
		}

		len -= i;
		if(!len)
		{
			return 0;
		}

		buf += i;
	}

	if(buf[0] == '*')
	{
		if(rargs)
		{
			rargs->type = RED_REQ_MULTI_BULK;
			rargs->nil = 0;
			rargs->args.clear();
		}
		ret = multi_bulk_parse(buf, len, rargs);
	}
	else
	{
		if(rargs)
		{
			rargs->type = RED_REQ_MULTI_INLINE;
			rargs->nil = 0;
			rargs->args.clear();
		}
		ret = inline_parse(buf, len, rargs);
	}

	if((ret > 0) && (i != -1))
	{
		ret += i;
	}

	return ret;
}

/*
	ret: 
	-1 err
	 0 need more bytes
	len success, total len
	
*/
int redis_reply_parse(const char* buf, int len, redis_args* rargs, unsigned long long &serial_num)
{
	int i, st, ret;
    serial_num = 0;
    
	if((!buf) || (len <= 0))
	{
		return -1;
	}

	if(rargs)
	{
		rargs->nil = 0;
		rargs->args.clear();
	}

	i = -1;

	if(buf[0] == '@')
	{
		/*ext sess ctx*/
		st = REP_SESS_DIGITAL;
		for(i = 1; (i<len) && (st!=REP_SESS_N); i++)
		{
			switch(st)
			{
			case REP_SESS_DIGITAL:

				switch(buf[i])
				{
					case '0' ... '9':
                        serial_num = serial_num * 10 + buf[i] - 48;
					case 'a' ... 'z':
					case '|':
					case '_':
						break;
					case '\r':
						if(i == 1)
						{
							return -1;
						}
						st = REP_SESS_R;
						break;
					default:
						return -1;
				}
				
				break;
			case REP_SESS_R:
				if(buf[i] != '\n')
				{
					return -1;
				}
				st = REP_SESS_N;
				break;
			}
		}

		if(st != REP_SESS_N)
		{
			return 0;
		}

		len -= i;
		if(!len)
		{
			return 0;
		}

		buf += i;
	}

#define __SET_TYPE(ptr, t) if(ptr) (ptr)->type = t

	switch(buf[0])
	{
		case '*':
			__SET_TYPE(rargs, RED_REPLY_MULTI_BULK);
			ret = multi_bulk_parse(buf, len, rargs);
			break;
			
		case '+':
			__SET_TYPE(rargs, RED_REPLY_STATUS);
			ret = line_msg_parse(buf, len, rargs);
			break;

		case '-':
			__SET_TYPE(rargs, RED_REPLY_ERR_MSG);
			ret = line_msg_parse(buf, len, rargs);
			break;
			
		case ':':
			__SET_TYPE(rargs, RED_REPLY_NUM);
			ret = line_msg_parse(buf, len, rargs);
			break;
			
		case '$':
			__SET_TYPE(rargs, RED_REPLY_BULK);
			ret = bulk_parse(buf, len, rargs);
			break;

		default:
			return -1;
	}

	if((ret > 0) && (i != -1))
	{
		ret += i;
	}

	return ret;
}


/*
int main()
{
	int ret, nil;
	const char* testbuf = "*1\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";//"*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";
	char buf[1024];
	redis_args rarg;

	ret = redis_cmd_parse(testbuf, strlen(testbuf), &rarg);

	printf("ret %d %d %d\n", ret, rarg.nil, rarg.type);

	if((ret > 0) && (!rarg.nil))
	{
		for(size_t sz=0; sz < rarg.args.size(); sz++)
		{
			if(rarg.args[sz].first <= 0)
			{
				printf("###|%d|###\n", rarg.args[sz].first);
			}
			else
			{
				memcpy(buf, rarg.args[sz].second, rarg.args[sz].first);
				buf[rarg.args[sz].first] = 0;
				printf("|%s|\n", buf);
			}
		}
	}

	testbuf = "*1\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";//"*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";

	ret = redis_reply_parse(testbuf, strlen(testbuf), &rarg);

	printf("ret %d %d %d\n", ret, rarg.nil, rarg.type);

	if((ret > 0) && (!rarg.nil))
	{
		for(size_t sz=0; sz < rarg.args.size(); sz++)
		{
			if(rarg.args[sz].first <= 0)
			{
				printf("###|%d|###\n", rarg.args[sz].first);
			}
			else
			{
				memcpy(buf, rarg.args[sz].second, rarg.args[sz].first);
				buf[rarg.args[sz].first] = 0;
				printf("|%s|\n", buf);
			}
		}
	}


	return 0;
}
*/

