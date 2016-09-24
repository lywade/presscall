#ifndef _HTTP_HPP_
#define _HTTP_HPP_

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define HTTPLABELCOUNT		50
#define FILE_PATH_MAX		256
#define MAX_HEAD_LEN		4096		//max size of http request or response header
#define MAX_URL_LENGTH		1024
#define RESPONSE_LEN	1500
#define RESPONSE_BODY_LEN	1024

#define GET_PARAMS_NUM	11

enum{ METHOD_GET, METHOD_HEAD, METHOD_POST };
enum{ RECODE_200 = 200, RECODE_206 = 206, RECODE_301 = 301, RECODE_302 = 302, RECODE_304 = 304, RECODE_401 = 401, RECODE_403 = 403, RECODE_404 = 404, RECODE_416 = 416, RECODE_500 = 500 };

struct LISTITEM
{
	void*				data;
	struct LISTITEM*	next;
};

struct LIST
{
	struct LISTITEM* head;
	struct LISTITEM* end;
};

struct HCONN
{
	unsigned short		recode;
	unsigned char		method;
	char*				url;
	char*				host;
	char*				accept;
	char*				referer;					//referer
	char*				language;					//字符语言
	char*				encoding;					//编码格式
	char*				range;						//range信息
	char*				cookie;						//cookie
	char*				servername;					//server_name
	char				keepalive;					//keep_alive
	char*				contenttype;				//content_type
	char*				disposition;				//disposition
	char*				location;					//重定向rul
	char				md5[64];					//文件md5值
	char*				banner;						//banner
	uint32_t			date;						//应答时间
	uint32_t			maxcache;					//cache时间
	uint32_t			expires;					//过期时间
	off64_t				postsize;					//发送数据大小
	uint32_t			headsize;					//包头大小
	off64_t				filesize;					//发送文件大小
	off64_t				returnsize;				//发送完成数据长度
	off64_t				rangeseek;					//读取文件range的起始位置
	time_t				mtime;						//文件最后修改时间
	off64_t				modifiedl;
	char*				modifieds;					//last modified
	char*					etag;					//entity tag
	
	struct vhost*		vhost;						//虚拟主机信息
	char*				data[HTTPLABELCOUNT];		//所有头信息， 当字段没有时可在这个数据里遍历
	char				localfile[FILE_PATH_MAX];	//本地文件路径及文件名
	struct LIST				varheader;						//基本header之外的header
};

using namespace std;

class Http
{
public:
	Http();
	~Http();
	static void init();
	static char* strtrim( char* str );
	static char* strpcpy( char* dest, const char* src );
	static void getval( char* str, char* pstr, char* end, char** val, unsigned short count );
	static char* getvalue( char** val, unsigned short count, char* str, short len );
	static void getpostval( char* str, char* pstr, char* end, char** val, unsigned short count );
	static int parse_label( struct HCONN* hconn, char* data, int req );
	static int http_request(struct HCONN *hconn , char* data, int len );
	static int http_parse(struct HCONN *hconn , char* data, int len );
	static int parse_responese( struct HCONN* hconn, char* data, int req );
	//static int ParseHttpRequest(ST_FWD_REQUEST *req_st , struct HCONN *hconn, char *req_buf , int req_len);
	static int make_request( struct HCONN* hconn, char* data, int* len );
	static int make_responses( struct HCONN* hconn, char* data, int* len  );
	static int getHttpResponseContent(struct HCONN *hconn, char *responsdata, int responslen,char *content);
	static int make_head( struct HCONN* hconn, char* data, int* len );
	static int php_htoi(char *s);
	static int urldecode(const char *str_source,char *out_str);
};


#endif

