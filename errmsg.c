#ifndef __ERRMSG_H__
#define __ERRMSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "errmsg.h"

struct errmsg
{
	const char *resp; int len;	/* response line */
	int code;
	const char *form;		/* form */
};

struct errmsg2
{
	const char *mime;
	char *part1;
	char *part2;
	int len1;
	int len2;
	int maxage;
};

#define ENTRY_RESP(i, s)	[i].resp = #i " " s, [i].len = sizeof(s)+3
#define ENTRY_RSP2(i, s)	[i].resp = s, [i].len = sizeof(s)-1
#define ENTRY_FORM(i, s)	[i].form = s
#define ENTRY_CODE(i, s)	[i].code = s


static const struct errmsg errmsg[600] = 
{
	ENTRY_RESP(100, "Continue"),
	ENTRY_RESP(101, "Switching Protocols"),
	ENTRY_RESP(102, "Processing"),
	ENTRY_RESP(200, "OK"),
	ENTRY_RESP(201, "Created"),
	ENTRY_RESP(202, "Accepted"),
	ENTRY_RESP(203, "Non-Authoritative Information"),
	ENTRY_RESP(204, "No Content"),
	ENTRY_RESP(205, "Reset Content"),
	ENTRY_RESP(206, "Partial Content"),
	ENTRY_RESP(300, "Multiple Choices"),
	ENTRY_RESP(301, "Moved Permanently"),
	ENTRY_FORM(301, "The actual URL is '%s'."),
	ENTRY_RESP(302, "Found"),
	ENTRY_FORM(302, "The actual URL is '%s'."),
	ENTRY_RESP(303, "See Other"),
	ENTRY_FORM(303, "The answer to your request is located '%s'."),
	ENTRY_RESP(304, "Not Modified"),
	ENTRY_RESP(305, "Use Proxy"),
	ENTRY_FORM(305, "This resource is only accessible through the proxy '%s'."),
	ENTRY_RESP(307, "Temporary Redirect"),
	ENTRY_FORM(307, "The actual URL is '%s'."),
	ENTRY_RESP(400, "Bad Request"),
	ENTRY_FORM(400, "Your request has bad syntax or is inherently impossible to satisfy."),
	ENTRY_RESP(401, "Authorization Required"),
	ENTRY_FORM(401, "Authorization required for the URL '%s'."),
	ENTRY_RESP(402, "Payment Required"),
	ENTRY_RESP(403, "Forbidden"),
	ENTRY_FORM(403, "You do not have permission to get URL '%s' from this server."),
	ENTRY_RESP(404, "Not Found"),
	ENTRY_FORM(404, "The requested URL '%s' was not found on this server."),
	ENTRY_RESP(405, "Method Not Allowed"),
	ENTRY_RESP(406, "Not Acceptable"),
	ENTRY_RESP(407, "Proxy Authentication Required"),
	ENTRY_FORM(407, "Authorization required for the URL '%s'."),
	ENTRY_RESP(408, "Request Time-out"),
	ENTRY_FORM(408, "No request appeared within a reasonable time period."),
	ENTRY_RESP(409, "Conflict"),
	ENTRY_RESP(410, "Gone"),
	ENTRY_FORM(410, "The requested URL '%s' was not found on this server, and there is no forwarding address."),
	ENTRY_RESP(411, "Length Required"),
	ENTRY_FORM(411, "The request method '%s' requires a valid Content-length."),
	ENTRY_RESP(412, "Precondition Failed"),
	ENTRY_RESP(413, "Request Entity Too Large"),
	ENTRY_FORM(413, "The amount data provided in request '%s' exceeds the capacity limit."),
	ENTRY_RESP(414, "Request-URI Too Large"),
	ENTRY_FORM(414, "The requested URL's length exceeds the capacity limit for this server."),
	ENTRY_RESP(415, "Unsupported Media Type"),
	ENTRY_FORM(415, "The supplied request data is not in a format acceptable for processing by this resource."),
	ENTRY_RESP(416, "Request Range Not Satisfiable"),
	ENTRY_RESP(417, "Expectation Failed"),
	ENTRY_FORM(417, "The expectation given in the Expect request-header field could not be met by this server."),
	ENTRY_RESP(422, "Unprocessable Entity"),
	ENTRY_FORM(422, "The server understands the media type of the request entity, but was unable to process the contained instructions."),
	ENTRY_RESP(423, "Locked"),
	ENTRY_FORM(423, "The requested resource is currently locked."),
	ENTRY_RESP(424, "Failed Dependency"),
	ENTRY_FORM(424, "The method could not be performed on the resource because the requested action depended on another action and that other action failed."),
	ENTRY_RESP(426, "Upgrade Required"),
	ENTRY_FORM(426, "The requested resource can only be retrieved using SSL."),
	ENTRY_RESP(500, "Internal Error"),
	ENTRY_FORM(500, "There was an unusual problem serving the requested URL '%s'."),
	ENTRY_RESP(501, "Method Not Implemented"),
	ENTRY_FORM(501, "The requested method '%s' is not implemented by this server."),
	ENTRY_RESP(502, "Bad Gateway"),
	ENTRY_FORM(502, "The proxy server received an invalid response from upstream server."),
	ENTRY_RESP(503, "Service Temporarily Unavailable"),
	ENTRY_FORM(503, "The requested URL '%s' is temporarily overloaded.  Please try again later."),
	ENTRY_RESP(504, "Gateway Time-out"),
	ENTRY_FORM(504, "The proxy server did not receive a timely response from the upstream server."),
	ENTRY_RESP(505, "HTTP Version Not Supported"),
	ENTRY_FORM(505, "This server only support HTTP version 1.0 and 1.1."),
	ENTRY_RESP(506, "Variant Also Negotiates"),
	ENTRY_FORM(506, "A variant for the requested resource '%s' is itself a negotiable resource."),
	ENTRY_RESP(507, "Insufficent Storage"),
	ENTRY_FORM(507, "There is insufficient free space left in your storage allocation."),
	ENTRY_RESP(510, "Not Extended"),
	ENTRY_FORM(510, "A mandatory extension policy in the request is not accepted by the server for this resource."),

	ENTRY_CODE(450, 403),	/* Bad Referer */
	ENTRY_RSP2(450, "403 Forbidden"),
	ENTRY_FORM(450, "You do not have permission to get URL '%s' from this server."),

	ENTRY_CODE(451, 403),	/* No Referer */
	ENTRY_RSP2(451, "403 Forbidden"),
	ENTRY_FORM(451, "You do not have permission to get URL '%s' from this server."),
};


const char* errmsgresp( int code )
{
	if( code >= 0 && code < sizeof( errmsg ) / sizeof( struct errmsg ) )
		return errmsg[code].resp;

	return "";
}

const char* errmsgform( int code )
{
	if( code >= 0 && code < sizeof( errmsg ) / sizeof( struct errmsg ) )
		return errmsg[code].form;

	return "";
}

#ifdef __cplusplus
}
#endif

#endif

