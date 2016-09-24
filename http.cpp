#include "http.hpp"
#include "errmsg.h"
#include "mytime.hpp"
#include <fstream>

using namespace std;

char* METHODTYPE[] = { "GET", "HEAD", "POST" };

Http::Http()
{
}

Http::~Http()
{
}


void Http::init()
{
}

char* Http::strtrim( char* str )
{
	if( str == NULL )
		return NULL;

	while( *str != 0x0 && *str == ' ') str++;
	char* val = str + strlen( str ) - 1;
	while( val > str && *val != 0x0 && *val == ' ' ) *val-- = 0x0;
	return str;
}

char* Http::strpcpy( char* dest, const char* src )
{
	while( ( *dest = *src++ ) != 0x0 )
		dest++;

	return dest;
}

void Http::getval( char* str, char* pstr, char* end, char** val, unsigned short count )
{
	char* pnext = NULL; unsigned short i = 0;
	for( i = 0; i < count && str != NULL; i++ )
	{
		val[i] = strtrim( strtok_r( str, pstr, &pnext ) );
		if( val[i] == NULL || ( end != NULL && strcmp( val[i], end ) == 0 ) ) break;
		str = pnext;
	}
	
}

char* Http::getvalue( char** val, unsigned short count, char* str, short len )
{
	unsigned short i = 0;
	for( i = 0; i < count && val[i] != NULL; i++ )
	{
		if( strncasecmp( val[i], str, len ) == 0 ){
			return val[i] + len;
		}
	}
	
	return NULL;
}

int Http::parse_label( struct HCONN* hconn, char* data, int req )
{
	char* str[3] = {NULL};
	getval( data, "\r\n", "", hconn->data, HTTPLABELCOUNT );
	char* line = hconn->data[0];
	if( line == NULL )
		return -1;

	getval( line, " ", NULL, str, 3 );
	if( req )
	{
		if( strncasecmp( str[0], "GET", 3 ) == 0 )
		{
			line += 4;
			hconn->method = METHOD_GET;
		}
		else if( strncasecmp( str[0], "HEAD", 4 ) == 0 )
		{
			line += 5;
			hconn->method = METHOD_HEAD;
		}
		else if( strncasecmp( str[0], "POST", 4 ) == 0 )
		{
			line += 5;
			hconn->method = METHOD_POST;
		}
		else return -1;
	}
	else
	{
		if( strncasecmp( str[0], "HTTP", 4 ) == 0 && str[1] != NULL ) hconn->recode = atoi( str[1] );
		else return -1;
	}

	hconn->url = str[1];
	hconn->host = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Host:", 5 ) );
	hconn->accept = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept:", 7 ) );
	hconn->encoding = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept-Encoding:", 16 ) );
	hconn->referer = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Referer:", 8 ) );
	hconn->language = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept-Language:", 16 ) );
	//hconn->encoding = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Content-Encoding:", 17 ) );
	line = getvalue( hconn->data, HTTPLABELCOUNT, "Connection:", 11 );
	hconn->keepalive = ( line != NULL && strcasecmp( strtrim( line ), "Close" ) == 0 ) ? 0 : 1;
	line = getvalue( hconn->data, HTTPLABELCOUNT, "Content-length:", 15 );
	hconn->postsize = line == NULL ? 0 : atoi( strtrim( line ) );
	hconn->cookie = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Cookie:", 7 ) );
	hconn->range = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Range: bytes=", 13 ) );
	line = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "If-Modified-Since:", 18 ) );
	if( line != NULL )
	{
		char* val[3] = {NULL};
		getval( line, ";", "", val, 3 );
		hconn->modifieds = val[0];
		line = strtrim( getvalue( val, 3, "length=", 7 ) );
		hconn->modifiedl = line == NULL ? 0 : atoll( line );
	}

	if( hconn->host != NULL )
	{
		line = strstr( hconn->host, ":" );
		if( line != NULL ) *line = 0x0;
	}
	return 0;
}


int Http::parse_responese( struct HCONN* hconn, char* data, int req )
{
	char* str[3] = {NULL};
	getval( data, "\r\n", "", hconn->data, HTTPLABELCOUNT );
	char* line = hconn->data[0];
	if( line == NULL )
		return -1;

	getval( line, " ", NULL, str, 3 );
	if( req )
	{
		if( strncasecmp( str[0], "GET", 3 ) == 0 )
		{
			line += 4;
			hconn->method = METHOD_GET;
		}
		else if( strncasecmp( str[0], "HEAD", 4 ) == 0 )
		{
			line += 5;
			hconn->method = METHOD_HEAD;
		}
		else if( strncasecmp( str[0], "POST", 4 ) == 0 )
		{
			line += 5;
			hconn->method = METHOD_POST;
		}
		else return -1;
	}
	else
	{
		if( strncasecmp( str[0], "HTTP", 4 ) == 0 && str[1] != NULL ) hconn->recode = atoi( str[1] );
		else return -1;
	}

	hconn->url = str[1];
	hconn->host = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Host:", 5 ) );
	hconn->accept = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept:", 7 ) );
	hconn->encoding = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept-Encoding:", 16 ) );
	hconn->referer = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Referer:", 8 ) );
	hconn->language = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Accept-Language:", 16 ) );
	//hconn->encoding = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Content-Encoding:", 17 ) );
	line = getvalue( hconn->data, HTTPLABELCOUNT, "Connection:", 11 );
	hconn->keepalive = ( line != NULL && strcasecmp( strtrim( line ), "Close" ) == 0 ) ? 0 : 1;
	line = getvalue( hconn->data, HTTPLABELCOUNT, "Content-length:", 15 );
	hconn->postsize = line == NULL ? 0 : atoi( strtrim( line ) );
	hconn->cookie = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Cookie:", 7 ) );
	hconn->range = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "Range: bytes=", 13 ) );
	line = strtrim( getvalue( hconn->data, HTTPLABELCOUNT, "If-Modified-Since:", 18 ) );
	if( line != NULL )
	{
		char* val[3] = {NULL};
		getval( line, ";", "", val, 3 );
		hconn->modifieds = val[0];
		line = strtrim( getvalue( val, 3, "length=", 7 ) );
		hconn->modifiedl = line == NULL ? 0 : atoll( line );
	}

	if( hconn->host != NULL )
	{
		line = strstr( hconn->host, ":" );
		if( line != NULL ) *line = 0x0;
	}
	return 0;
}

/*check http request from client*/
int Http::http_parse(struct HCONN *hconn , char* data, int len )
{
	if( len < 4 ) return 0;
	char* pend = strstr( data, "\r\n\r\n" );
	if( pend == NULL ) return len < MAX_HEAD_LEN ? 0 : -1;
	hconn->headsize = pend - data + 4;
	char preq_buf[MAX_HEAD_LEN] = {"\0"};
	memcpy(preq_buf, data, hconn->headsize);
	if( hconn->headsize >= MAX_HEAD_LEN - 1 ) return -1;
	if( parse_label( hconn, preq_buf, 0 ) != 0 ) return -1;
	if( len < ( hconn->headsize + hconn->postsize ) ) return 0;
	return hconn->headsize + hconn->postsize;
}


/*check http request from client*/
int Http::http_request(struct HCONN *hconn , char* data, int len )
{
	if( len < 4 ) return 0;
	char* pend = strstr( data, "\r\n\r\n" );
	if( pend == NULL ) return len < MAX_HEAD_LEN ? 0 : -1;
	hconn->headsize = pend - data + 4;
	char preq_buf[MAX_HEAD_LEN] = {"\0"};
	memcpy(preq_buf, data, hconn->headsize);
	if( hconn->headsize >= MAX_HEAD_LEN - 1 ) return -1;
	if( parse_label( hconn, preq_buf, 1 ) != 0 ) return -1;
	if( len < ( hconn->headsize + hconn->postsize ) ) return 0;
	return hconn->headsize + hconn->postsize;
}

int Http::getHttpResponseContent(struct HCONN *hconn, char *responsdata, int responslen,char *content){
	int ret = 0;
	ret = http_parse( hconn, responsdata , responslen);
	if(ret <0)
	{
		printf("http_request error \n");
		return -1;
	}
	memcpy(content ,responsdata + hconn->headsize, hconn->postsize);
	return 0;
}

/*parse http request*/
//解析得到url的参数以及post过来的content
/*int Http::ParseHttpRequest(ST_FWD_REQUEST *req_st , struct HCONN *hconn, char *req_buf , int req_len)
{
	int ret; 
	char *pos;
	char *t_param;
	char* data[GET_PARAMS_NUM] = {NULL};
	if(!req_buf || !hconn)
	{
		return -1;
	}
	//INFO("INFO:Recv request =%s\n" , req_buf);

	memset(hconn , 0 , sizeof(struct HCONN));
	if((ret = http_request( hconn, req_buf , req_len)) <0)
	{
		return -1;
	}
	//if the method is post give the content
	if(hconn->method == METHOD_POST)
	{
		req_st->content.append(req_buf + hconn->headsize, hconn->postsize);
	}
	//get the params
	if(hconn->url)
	{
		char urldata[MAX_URL_LENGTH] = {"\0"};
		urldecode(hconn->url, urldata);
		strcpy(hconn->url,urldata);
		pos = strstr(hconn->url , "/");
		if(!pos)
		{
			return -1;
		}
		getval(pos+1 , "?" , "" , data , GET_PARAMS_NUM);
		req_st->reqtype = data[0];
		pos = data[1];
		getval(pos , "&" , "" , data , GET_PARAMS_NUM);
		
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "bid=" , 4))) != NULL)
			req_st->bid = atoi(t_param);
		else
			req_st->bid = 0;
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "did=" , 4))) != NULL)
			req_st->did = t_param;
		else req_st->did = "";
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "offlineTime=" , 12))) != NULL)
			req_st->offlineTime = atoi(t_param);
		else req_st->offlineTime = 0;
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "cltVersion=" , 11))) != NULL)
			req_st->cltVersion = atoi(t_param);
		else req_st->cltVersion = 0;
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "delete=" , 7))) != NULL)
			req_st->del = atoi(t_param);
		else req_st->del = 1;
		if((t_param = strtrim(getvalue(data , GET_PARAMS_NUM , "seq=" , 4))) != NULL)
			req_st->seq = t_param;
		else req_st->seq = "";
	}

	else
	{
		return -1;
	}

	//verify the refer
	int refer_ver_flag;
	char refer_ver_value[4] = {'\0'};
	
	refer_ver_flag = atoi(refer_ver_value);

	if(refer_ver_flag)
	{
		if(hconn->referer)
		{
			if(!strstr(hconn->referer , "qq.com"))
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}

	return 0;

}
*/
/*make the http request*/
int Http::make_request( struct HCONN* hconn, char* data, int* len )
{
	char* str = data;
	data += sprintf( data, "%s %s HTTP/1.1\r\n", METHODTYPE[hconn->method], hconn->url );
	data += sprintf( data, "Host: %s\r\n", hconn->servername );
	if( hconn->accept != NULL ) data += sprintf( data, "Accept: %s\r\n", hconn->accept );
	if( hconn->cookie != NULL ) data += sprintf( data, "Cookie: %s\r\n", hconn->cookie );
#if defined __x86_64__
	data += sprintf( data, "Content-Length: %ld\r\n", hconn->postsize );
#else
	data += sprintf( data, "Content-Length: %lld\r\n", hconn->postsize );
#endif
	data += sprintf( data, "Content-Type: %s\r\n", hconn->contenttype );
	data += sprintf( data, "Connection: %s\r\n", hconn->keepalive == 1 ? "Keep-Alive" : "Close" );
	data += sprintf( data, "Content-Encoding: %s\r\n", hconn->encoding );
	strcpy( data, "\r\n" ); data += 2;
	if( len != NULL ) *len = data - str;
	hconn->headsize = data - str;
	return 0;
}

/*make the http response*/
int Http::make_responses( struct HCONN* hconn, char *data, int *len)
{ 
	char* str = data;
	data = strpcpy( data, "HTTP/1.1 " );
	data += sprintf( data, errmsgresp( hconn->recode ), hconn->url );
	data = strpcpy( data, "\r\n" );
	if( hconn->servername) data += sprintf( data, "Server: %s\r\n", hconn->servername );
	string kstr = hconn->keepalive == 1 ? "keep-alive" : "close";
	data += sprintf( data, "Connection: %s\r\n", kstr.c_str());
	data = strpcpy( data, "Date: " );
	nowtimestr( data );
	data = data + strlen( data );
	data = strpcpy( data, "\r\n" );
	if( hconn->expires != 0 )
	{
		data = strpcpy( data, "Expires: " );
		gettimestr( data, hconn->expires );
		data = data + strlen( data );
		data = strpcpy( data, "\r\n" );
	}

	data += sprintf( data, "Cache-Control: max-age=%d\r\n", hconn->maxcache );
	if( hconn->mtime != 0 )
	{
		data = strpcpy( data, "Last-Modified: " );
		gettimestr( data, hconn->mtime );
		data = data + strlen( data );
		data = strpcpy( data, "\r\n" );
	}

	if( hconn->contenttype != NULL ) data += sprintf( data, "Content-Type: %s\r\n", hconn->contenttype );
#if defined __x86_64__
	data += sprintf( data, "Content-Length: %ld\r\n", hconn->postsize );
#else
	data += sprintf( data, "Content-Length: %lld\r\n", hconn->postsize );
#endif
	if( hconn->disposition != NULL ) data += sprintf( data, "Content-Disposition: attachment; filename=%s\r\n", hconn->disposition );
	if( hconn->md5[0] != 0x0 ) data += sprintf( data, "Content-MD5: %s\r\n", hconn->md5 );
	if( hconn->location != NULL ) data += sprintf( data, "Location: %s\r\n", hconn->location );
	if( hconn->encoding != NULL && strcmp( hconn->encoding, "gzip" ) == 0 ) data += sprintf( data, "Content-Encoding: %s\r\n", hconn->encoding );
	if( hconn->range != NULL ) 
#if defined __x86_64__
		data += sprintf( data, "Accept-Ranges: bytes\r\nContent-Range: bytes %ld-%ld/%ld\r\n", hconn->rangeseek, hconn->rangeseek + hconn->postsize, hconn->filesize );
#else
		data += sprintf( data, "Accept-Ranges: bytes\r\nContent-Range: bytes %lld-%lld/%lld\r\n", hconn->rangeseek, hconn->rangeseek + hconn->postsize, hconn->filesize );	
#endif
	/*struct LISTITEM* item = list_getfirst( &hconn->varheader);
	while( item != NULL )
	{
		struct HttpHeader* header = (struct HttpHeader*)item->data;
		if( header && header->key[0] && header->value[0])
		{
			//Attention, since data is fixed-length, there is a chance of memory overflow!
			data+=sprintf(data, "%s:%s\r\n", header->key, header->value);
		}
		item = list_getnext( item );
	}*/

	data = strpcpy( data, "\r\n" );
	if( len != NULL ) *len = data - str;
	hconn->headsize = data - str;
	return 0;
}

/*make the http head*/
int Http::make_head( struct HCONN* hconn, char* data, int* len )
{
	char* str = data; unsigned short i = 0;
	data += sprintf( data, "%s %s HTTP/1.1\r\n", METHODTYPE[hconn->method], hconn->url );
	for( i = 1; i < HTTPLABELCOUNT && hconn->data[i] != NULL; i++ )
	{
		data += sprintf( data, "%s\r\n", hconn->data[i] );
	}
	strcpy( data, "\r\n" ); data += 2;
	if( len != NULL ) *len = data - str;
	hconn->headsize = data - str;
	return 0;
}


int Http::php_htoi(char *s)  
{  
    int value;  
    int c;  
  
    c = ((unsigned char *)s)[0];  
    if (isupper(c))  
        c = tolower(c);  
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;  
  
    c = ((unsigned char *)s)[1];  
    if (isupper(c))  
        c = tolower(c);  
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;  
  
    return (value);  
}  
  
/*  
   用法：int urldecode(const char* str_source,char *out_str) 
*/  
int Http::urldecode(const char *str_source,char *out_str)  
{  
    char const *in_str = str_source;  
    int in_str_len = strlen(in_str);  
    char *str;  
    int out_str_len = 0;
    str = strdup(in_str);  
    char *dest = str;  
    char *data = str;  
  
    while (in_str_len--) {  
        if (*data == '+') {  
            *dest = ' ';  
        }  
        else if (*data == '%' && in_str_len >= 2 && isxdigit((int) *(data + 1))   
            && isxdigit((int) *(data + 2))) {  
                *dest = (char) php_htoi(data + 1);  
                data += 2;  
                in_str_len -= 2;  
        } else {  
            *dest = *data;  
        }  
        data++;  
        dest++;  
    }  
    *dest = '\0';  
    out_str_len =  dest - str;  
    strncpy(out_str, str, out_str_len);  
    free(str);  
    return 0;
}


