#include "user_func.h"
#include "TSocket.h"
#include "redis_proto_parser.h"
#include "hash.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/tcp.h>
//#include <asm/atomic.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <string.h>
#include <limits.h>
#include <string>
#include <map>
#include "buffer.h"
#include "loadgrid.h"

#define BUF_LEN 16777216

#if __cplusplus >= 201103L
#include <random>

static std::random_device rdv;
static std::mt19937 gen(rdv());
static std::uniform_int_distribution<long long> rnd(0, 0);

#endif

using namespace std;

#define WEB_CS_PREFIX 			((uint8_t)0x55)
#define WEB_CS_SUFIX			((uint8_t)0xAA)
#define PACKET_RECV_MINLEN		((int)(sizeof(uint8_t) + sizeof(uint32_t)))
#define random(x) (rand()%x)
#define range(x) ((long long)(rand() * rand())%x)


int net_complete_func(const void* pData, unsigned int unDataLen, int &iPkgTheoryLen)
{
	iPkgTheoryLen = 0;
	if (unDataLen < sizeof(int))
		return 0;

	int iMsgLen = ntohl(*(int*)pData);
	iPkgTheoryLen = iMsgLen;

	if (iPkgTheoryLen <= (int)unDataLen)
	{
		return iPkgTheoryLen;
	}

	return 0;
}

void array_append(int array[], int pos, int num, int val)
{
    for(int i=pos; i<(pos+num); i++){
        array[i] = val;
    }
}


CUserFunc::CUserFunc(int socket, struct TConfig g_Config)
{
    m_sockfd = socket;
    assert(m_sockfd >= 0);

    gettimeofday(&tBase, NULL);
    iVlen = g_Config.m_iValueLength;
    iFlen = g_Config.m_iValueLength;
    iFnum = g_Config.m_iFieldNumber;
    iLev1 = g_Config.m_iTimeLevel1;
    iLev2 = g_Config.m_iTimeLevel2;
    iLev3 = g_Config.m_iTimeLevel3;
    cmd_percent = g_Config.m_CommandPercent;

    if(cmd_buf == NULL)
        cmd_buf = (char *)malloc(BUF_LEN * sizeof(char));

    pLoadGrid = new CLoadGrid(10000, 50, g_Config.m_iMaxReqNumber*10);
}


CUserFunc::~CUserFunc()
{
    free(cmd_buf);
    delete pLoadGrid;
}

/*
//呼叫结果元素
struct  TUsrResult
{
//总呼量
int iAllReqNum;
int iOkResponseNum;
int iNoResponseNum;
int iBadResponseNum;
};
return: 非0则sleep(1),0则继续
*/

void CUserFunc::WriteSocket(struct TUsrResult *pResult)
{
    size_t n;
    if(w_Buffer.readableBytes() == 0){
		WriteCommandToBuf(pResult);
    }
	
	n = write(m_sockfd, w_Buffer.peek(), w_Buffer.readableBytes());
	if(n<=0 && errno!=EINTR){
		printf("write fd error\n");
		exit(0);
	}
    if(n > 0){
        w_Buffer.retrieve(n);
    }
}


void CUserFunc::ReadSocket(struct TUsrResult *pResult)
{
    char extrabuf[1048576];
    struct iovec vec[2];
    size_t writable = r_Buffer.writableBytes();
    vec[0].iov_base = r_Buffer.begin() + r_Buffer.writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    size_t n = readv(m_sockfd, vec, iovcnt);

    if(n<=0 && errno!=EINTR){
        if(n==0)
            printf("fd closed by remote peer\n");
        else
            printf("read fd error, msg:%s\n", strerror(errno));
        exit(0);
    }
    else if (n <= writable)
    {
        r_Buffer.writerIndex_ += n;
    }
    else
    {
        r_Buffer.writerIndex_ = r_Buffer.buffer_.size();
        r_Buffer.append(extrabuf, n - writable);
    }

    unsigned long long recv_num;
    redis_args rarg;
    int ret = 0;
    
    do{
        ret = redis_reply_parse(r_Buffer.peek(), r_Buffer.readableBytes(), &rarg, recv_num);
        if(ret <= 0)
            break;
        else{
            r_Buffer.retrieve(ret);
            pResult->iOkResponseNum++;
            struct timeval tRecv;
            gettimeofday(&tRecv, NULL);
        	pResult->ullRspTimeUs = (tRecv.tv_sec - tBase.tv_sec)*1000000 + 
                                    (tRecv.tv_usec - tBase.tv_usec) - recv_num;
            
            if(pResult->ullRspTimeUs > pResult->ullMaxRspTimeUs)
                pResult->ullMaxRspTimeUs = pResult->ullRspTimeUs;
            
            pResult->ullAllRspTimeUs += pResult->ullRspTimeUs;
            
            if(pResult->ullRspTimeUs >= iLev3)
		    {
			    pResult->m_iTimeL3Num++;
		    }
		    if((pResult->ullRspTimeUs < iLev3) && (pResult->ullRspTimeUs >= iLev2))
		    {
			    pResult->m_iTimeL2Num++;
		    }
		    if((pResult->ullRspTimeUs < iLev2) && (pResult->ullRspTimeUs >= iLev1))
		    {
			    pResult->m_iTimeL1Num++;
		    }
            
        }

        rarg.args.clear();
    }while(ret > 0);
}


void CUserFunc::WriteCommandToBuf(struct TUsrResult *pResult)
{
    struct timeval tSend;
    while(true)
    {
        //如果w_buffer已有一定量的待发命令，则立即返回
        if(w_Buffer.readableBytes() > 1024)
            break;

        gettimeofday(&tSend, NULL);
        //流量控制已满
        if(pLoadGrid->CheckLoad(tSend, false) == 1)
            break;

        size_t cmd_len = genCommand(tSend);
        w_Buffer.append(cmd_buf, cmd_len);   

    }

}



/*  desc: write command to cmd_buf once
 *
 *  command type
 * 	1: set
 * 	2: get
 *  3: setex
 * 	4: setnx
 * 	5: hmset
 * 	6: hmget
 * 	7: hgetall
 * 	8: del
 *  9: expire
 * 10: zadd
 * 11: zcard
 * 12: zrem
 * 13: zrange
 * 14: zrangebyscore
 *
 *
 * 	iSeq:	1 sequence ; 0 random
 * 	iKlen:	key length
 * 	iVlen:	value length
 * 	iFlen:	field length
 * 
 */
int CUserFunc::genCommand(struct timeval tSend, struct TUsrResult *pResult)
{
    pResult->iAllReqNum++;
    size_t iCmdLen = 0;
    int iType;
    int count = 0;

    int percent_array[200];
    memset(percent_array, 0, 200*sizeof(int));
    
    map<string, int>::iterator it;
    for(it=cmd_percent.begin(); it!=cmd_percent.end(); it++)
    {
        string name_percent = it->first;
        int num_percent = it->second;

        if(name_percent == "set"){
            array_append(percent_array, count, num_percent, 1);
        }
        else if(name_percent == "get"){
            array_append(percent_array, count, num_percent, 2);
        }
        else if(name_percent == "setex"){
            array_append(percent_array, count, num_percent, 3);
        }
        else if(name_percent == "setnx"){
            array_append(percent_array, count, num_percent, 4);
        }
        else if(name_percent == "hmset"){
            array_append(percent_array, count, num_percent, 5);
        }
        else if(name_percent == "hmget"){
            array_append(percent_array, count, num_percent, 6);
        }
        else if(name_percent == "hgetall"){
            array_append(percent_array, count, num_percent, 7);
        }
        else if(name_percent == "del"){
            array_append(percent_array, count, num_percent, 8);
        }
        else if(name_percent == "expire"){
            array_append(percent_array, count, num_percent, 9);
        }
        else if(name_percent == "zadd"){
            array_append(percent_array, count, num_percent, 10);
        }
        else if(name_percent == "zcard"){
            array_append(percent_array, count, num_percent, 11);
        }
        else if(name_percent == "zrem"){
            array_append(percent_array, count, num_percent, 12);
        }
        else if(name_percent == "zrange"){
            array_append(percent_array, count, num_percent, 13);
        }
        else if(name_percent == "zrangebyscore"){
            array_append(percent_array, count, num_percent, 14);
        }
        else 
            continue;
        
        count += num_percent;
    }

    int iRnd = random(count);
    iType = percent_array[iRnd];

    unsigned long long serial_num = (tSend.tv_sec - tBase.tv_sec)*1000000 +
                                    (tSend.tv_usec - tBase.tv_usec);

    iCmdLen = ProtocolMaker(iType, serial_num);
    return iCmdLen;

}


/* desc: used for test ssdb performance 
 * input: type
 * 	key/value
 * 	1: set
 * 	2: get
 *  3: setex
 * 	4: setnx
 * 	5: hmset 
 * 	6: hmget
 * 	7: hgetall
 * 	8: del
 *  9: expire
 * 10: zadd
 * 11: zcard
 * 12: zrem
 * 13: zrange
 * 14: zrangebyscore
 *
 *
 * 	iSeq:	1 sequence ; 0 random
 * 	iKlen:	key length
 * 	iVlen:	value length
 * 	iFlen:	field length 
 */
int CUserFunc::ProtocolMaker(int type, unsigned long long serial_num)
{
	static __thread int iRandSeq = 0;
	if(++iRandSeq > 1000)
	{
		iRandSeq = 0;
		srand(time(NULL));
	}

	string sFmt;
	this->genFormat(type, sFmt, serial_num);

	int len = 0;
    int ttl_sec = 600;
	string name, key, value, field;

	switch(type)
	{
		case 1: /*set key value*/
            this->getKey(1, 1, name);
			this->getValue(iVlen, value);
			len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), value.length(), value.c_str());
			break;
		case 2: /*get key*/
            this->getKey(0, 1, name);
			len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str());
			break;
        case 3: /*setex key ttl value*/ {
                this->getKey(1, 1, name);
                this->getValue(iVlen, value);
                char ttl[20];
                int ttl_len = sprintf(ttl, "%d", ttl_sec);
                len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), ttl_len, ttl, value.length(), value.c_str());
                break;
            }
        case 4: /*setnx key value*/
            this->getKey(1, 1, name);
            this->getValue(iVlen, value);
            len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), value.length(), value.c_str());
            break;
		case 5:	/*hmset name field value*/ {
		        return this->genMultiField(type, sFmt, name, len);
		    }
		case 6: /*hmget name field*/ {
		        return this->genMultiField(type, sFmt, name, len);
		    }
		case 7: /*hgetall name*/ 
            this->getKey(0, 1, name);
			len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str());	
			break;
        case 8: /*del key*/
            this->getKey(0, 1, name);
            len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str());
            break;
        case 9: /*expire key ttl*/ {
                char ttl[20];
                int ttl_len = sprintf(ttl, "%d", ttl_sec);
                this->getKey(0, 1, name);
                len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), ttl_len, ttl);
                break;
            }
        case 10: /*zadd key score value*/ {
                return this->genMultiField(type, iVlen, iFlen, iFnum, sFmt, name, wbuf, len);
            }
        case 11: /*zcard key*/
            this->getKey(0, 1, name);
			len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str());
			break;
        case 12: /*zrem key value*/ {
                return this->genMultiField(type, iVlen, iFlen, iFnum, sFmt, name, wbuf, len);
            }
        case 13: /*zrange key 0 -1*/ {
                char start[20], end[20];
                int s = sprintf(start, "%d", 0);
                int e = sprintf(end, "%d", -1);
                this->getKey(0, 1, name);
                len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), s, start, e, end);
                break;
            }
        case 14: /*zrangebyscore key min max*/ {
                char start[20], end[20];
                int max_score = INT_MAX;
                int s = sprintf(start, "%d", 0); 
                int e = sprintf(end, "%d", max_score);
                this->getKey(0, 1, name);
                len = sprintf(cmd_buf, sFmt.c_str(), name.length(), name.c_str(), s, start, e, end);
                break;
            }
		default:
			printf("error type , exit \n");
			exit(0);
	}

	return len;
}



/* desc:
 * used to generate value
 * */

void CUserFunc::getValue(int length , string &value){
	static __thread char base[] ="qw8tj90hgfm?:*'er.`yui6oplk@%dsa&(|=~#^_+124zxcv3bn57/";	
	char c;
	int pos = random(43);
    int iBase = 8;
	value.clear();
	//if(pos==0) iBase=25;

	int iCnt = length/iBase;
	int iMod = length%iBase;
	for(int i=0 ; i<iCnt ; i++)
	{
		value.append(&base[pos],iBase);
		pos = random(43);
	}
	if(iMod)
		value.append(base , iMod);

#ifdef DLENGTH
	if(value.length()!= length)
	{
		printf("val:%d len:%d pos:%d base:%d  icnt:%d mod:%d\n",value.length(),length , pos , iBase , iCnt , iMod);
		exit(0);
	}
#endif
	int iRnd = random(52);
	c = base[pos];
	base[pos] = base[iRnd];
	base[iRnd] = c;
}

/* desc:
 * used to generate field
 * */
void CUserFunc::getField(int length , string &field){
	static char base[] ="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";	
	char c;
	int pos = random(26);
        int iBase = random(26);
	field.clear();
	if(iBase == 0)
		iBase = 26;

	field.assign("field_");
	length -= field.length();
	if(length <= 0)
		return ;
	
	int iCnt = length/iBase;
	int iMod = length%iBase;

	for(int i=0 ; i<iCnt ; i++)
		field.append(&base[pos] , iBase);
	if(iMod)
		field.append(base , iMod);


	int iRnd = random(52);
	c = base[pos];
	base[pos] = base[iRnd];
	base[iRnd] = c;
}


/* desc: used to generate key
 * input: 
 * key_type
 *  1: write
 *  0: read
 * sequence 	
 *  0: random
 *  1: seqkey
 * return:
 *  0: success
 *  1: error
 * */
int CUserFunc::getKey(int isWrite, int isSeq, string &key)
{
	char buf[21];
	static int icnt = 0;
	static long long  iMaxSeq = 0;
	static long long  iMinSeq = INT_MAX;
	static long long  iwSeq = 0;
	static long long  irSeq = 0;
	key.clear();
    
	if(isSeq){
		if(isWrite){
			long long val = __sync_fetch_and_add(&iwSeq, 1);
 			val=val%800000;
			sprintf(buf, "test_%015lld", val);
			if(iwSeq > iMaxSeq)
				iMaxSeq = iwSeq;
			key.assign(buf);
		}
		else
		{	
			if(irSeq == INT_MAX)
				irSeq = 0;
			long long val = __sync_fetch_and_add(&irSeq, 1);
			sprintf(buf, "test_%015lld", val);
			key.assign(buf);
		}
	}
    else{
        long long rd = random(INT_MAX);
		if(rd > iMaxSeq)
			iMaxSeq = rd;
		if(rd < iMinSeq)
			iMinSeq = rd;
		if(++icnt%50000 == 0)
		{
			//printf("Max Rand:%lld\n",iMaxSeq);
			//printf("Min Rand:%lld\n",iMinSeq);
			iMaxSeq = 0;
			iMinSeq = LLONG_MAX;
		}
		sprintf(buf, "test_%015lld", rd);
		key.assign(buf);
/*        
#if __cplusplus < 201103L 
		long long rd = random(INT_MAX);
		if(rd > iMaxSeq)
			iMaxSeq = rd;
		if(rd < iMinSeq)
			iMinSeq = rd;
		if(++icnt%50000 == 0)
		{
			//printf("Max Rand:%lld\n",iMaxSeq);
			//printf("Min Rand:%lld\n",iMinSeq);
			iMaxSeq = 0;
			iMinSeq = LLONG_MAX;
		}
		sprintf(buf, "test_%015lld", rd);
		key.assign(buf);
#else
		long long rd = rnd(gen);	
		if(rd > iMaxSeq)
			iMaxSeq = rd;
		if(rd < iMinSeq)
			iMinSeq = rd;
		if(++icnt%100000 == 0)
		{
		//	printf("Max Rand:%lld\n",iMaxSeq);
		//	printf("Min Rand:%lld\n",iMinSeq);
			iMaxSeq = 0;
			iMinSeq = LLONG_MAX;
		}
		sprintf(buf, "test_%015lld", rnd(gen));
		key.assign(buf);
#endif
*/
	}
	return 0;
}


int CUserFunc::genMultiField(int type, string &sFmt, string name, int len)
{
	int score;
	char buf[20];
	string field, value;

    this->getKey(0, 1, name);
    sprintf(cmd_buf, "$%ld\r\n%s\r\n", name.length(), name.c_str());
    sFmt.append(cmd_buf);
        
	for(int i=0; i<iFnum; i++)
	{
		if(type == 5)
		{
		    this->getKey(1, 1, name);
			this->getField(iFlen, field);
			this->getValue(iVlen, value);
			sprintf(cmd_buf, "$%ld\r\n%s\r\n$%ld\r\n%s\r\n", field.length(), field.c_str(), value.length(), value.c_str());
		}
		else if(type == 6)
		{
			this->getField(iFlen, field);
			sprintf(cmd_buf, "$%ld\r\n%s\r\n", field.length(), field.c_str());
		}
        else if(type == 10)
        {               
            this->getField(iFlen, field);
            score = random(INT_MAX);
            int len = sprintf(buf, "%d", score);
            sprintf(cmd_buf, "$%d\r\n%s\r\n$%ld\r\n%s\r\n", len, buf, field.length(), field.c_str());
        }
        else if(type == 12)
		{
			this->getField(iFlen, field);
			sprintf(cmd_buf, "$%ld\r\n%s\r\n", field.length(), field.c_str());
		}
		sFmt.append(cmd_buf);
	}
    
	len = sFmt.length();
    strcpy(cmd_buf, sFmt.c_str());
    //printf("%s",sFmt.c_str());
	return len;

}


void CUserFunc::genFormat(int type, string &sFmt, unsigned long long serial_num)
{
    sprintf(cmd_buf, "@%lld\r\n", serial_num);
    sFmt.assign(cmd_buf);
    
	switch(type)
	{
		case 1:
			sFmt.append("*3\r\n$3\r\nset\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
			break;
		case 2:
			sFmt.append("*2\r\n$3\r\nget\r\n$%d\r\n%s\r\n");
			break;
        case 3:
			sFmt.append("*4\r\n$5\r\nsetex\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
            break;
        case 4:
            sFmt.append("*3\r\n$5\r\nsetnx\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
            break;          
		case 5:
			sprintf(cmd_buf, "*%d\r\n$5\r\nhmset\r\n", iFnum*2 + 2);
			sFmt.append(cmd_buf);
			break;
        case 6:
			sprintf(cmd_buf, "*%d\r\n$5\r\nhmget\r\n", iFnum + 2);
			sFmt.append(cmd_buf);
			break;
        case 7:
            sFmt.append("*2\r\n$7\r\nhgetall\r\n$%d\r\n%s\r\n");
            break;
		case 8:
			sFmt.append("*2\r\n$3\r\ndel\r\n$%d\r\n%s\r\n");	
			break;
        case 9:
            sFmt.append("*3\r\n$6\r\nexpire\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
            break;
        case 10:
            sprintf(cmd_buf, "*%d\r\n$4\r\nzadd\r\n", iFnum*2 + 2);
            sFmt.append(cmd_buf);
            break;
        case 11:
            sFmt.append("*2\r\n$5\r\nzcard\r\n$%d\r\n%s\r\n");
            break;
        case 12:
			sprintf(cmd_buf, "*%d\r\n$4\r\nzrem\r\n", iFnum + 2);
			sFmt.append(cmd_buf);
            break;
        case 13:
            sFmt.append("*4\r\n$6\r\nzrange\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
            break;
        case 14:
            sFmt.append("*4\r\n$13\r\nzrangebyscore\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n$%d\r\n%s\r\n");
            break;
		default:
			printf("unwanted type...\n");
			exit(-1);
	}
}


//生成指定随机key
string CUserFunc::randomgetkey(int type){
	const char *key_prefix[4] = {"xxxx_zz" , "test_yy" , "test_list" , "test_hash"};
	char key[64] = {0};
	type = random(4);
	int i = random(10000);
	sprintf(key , "%s_%d" , key_prefix[type],  i);
	
	return key;
}

//生成指定随机key
string CUserFunc::randomgetfiled(){
	string hash_filed[10] = {"hash_filed_1", "hash_filed_2", "hash_filed_3", "hash_filed_4", "hash_filed_5", "hash_filed_6", "hash_filed_7", "hash_filed_8", "hash_filed_9", "hash_filed_10"};
	int i = random(10);
	return hash_filed[i];
}


//生成指定长度的a-z的随机字符串
string CUserFunc::randomgetstr(int len){
	int step = 0;
	step = random(26);
	char a = 'a'+step;

	string randomstr(len , a);
	/*
	for(int i=0 ; i < len; i++ ){
		step = random(26);
		char a = 'a'+step;
		randomstr += a;
	}	
	*/
	return randomstr;
}

//得到一个1-10随机的数字
string CUserFunc::randomgetnumstr(){
	string randomnum[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
	int i = random(10);
	return randomnum[i];
}


int CUserFunc::PassportMsg(string cmd, string ticket, string project_code, string group, string type, int ars_num)
{
	int iBuffLen = 0;
	char *tmp = new char[1024];
	char *msgbody = new char[256];
	memset(tmp , 0, sizeof(tmp));
	memset(msgbody , 0, sizeof(msgbody));
	string v1 = cmd;
	unsigned int v1_len = htonl ( v1.length() );
	string v2 = ticket;
	unsigned int v2_len = htonl ( v2.length() );
	unsigned int v3_len = 0, v4_len = 0, v5_len = 0;
	if( ars_num >= 3 ){
		v3_len = htonl ( project_code.length() );
	}
	if( ars_num >= 4 ){
		v4_len = htonl ( group.length() );
	}
	if( ars_num >= 5 ){
		v5_len = htonl ( type.length() );
	}
	int bodylen = 0;
	msgbody[0] = 0x01;
	memcpy(msgbody+1, &v1_len, sizeof(v1_len));
	memcpy(msgbody+5, v1.c_str(), v1.length());
	int v2_pos = 5 + v1.length();
	msgbody[ v2_pos ] = 0x02;
	memcpy(msgbody+v2_pos+1, &v2_len, sizeof(v2_len));
	memcpy(msgbody+v2_pos+5, v2.c_str(), v2.length());
	bodylen = v2_pos+5+v2.length();
	int v3_pos = 0, v4_pos = 0, v5_pos = 0;
	if( ars_num >= 3 ){
		v3_pos = v2_pos+5 + v2.length();
		msgbody[ v3_pos ] = 0x03;
		memcpy(msgbody+v3_pos+1, &v3_len, sizeof(v3_len));
		memcpy(msgbody+v3_pos+5, project_code.c_str(), project_code.length());
		bodylen = v3_pos+5+project_code.length();
	}
	if( ars_num >= 4 ){
		v4_pos = v3_pos+5 + project_code.length();
		msgbody[ v4_pos ] = 0x04;
		memcpy(msgbody+v4_pos+1, &v4_len, sizeof(v4_len));
		memcpy(msgbody+v4_pos+5, group.c_str(), group.length());
		bodylen = v4_pos+5+group.length();
	}
	if( ars_num >= 5 ){
		v5_pos = v4_pos+5 + group.length();
		msgbody[ v5_pos ] = 0x05;
		memcpy(msgbody+v5_pos+1, &v5_len, sizeof(v5_len));
		memcpy(msgbody+v5_pos+5, type.c_str(), type.length());
		bodylen = v5_pos+5+type.length();
	}
	
	unsigned int beforebodylen = htonl ( bodylen + 5 );
	tmp[0] = 0x01;
	memcpy(tmp+1, &beforebodylen, sizeof(beforebodylen));
	memcpy(tmp+5, msgbody, bodylen);
	
	TcpCltSocket stTcpCltSocket;
	char DestIp[16];
	int iDestPort = 5862;
	sprintf(DestIp,"10.193.17.14");
	if(!stTcpCltSocket.ConnectServer(inet_addr(DestIp), iDestPort))
	{
		printf("connect %s:%d failed!\n",DestIp,iDestPort);
		return -1;
	}
	
	stTcpCltSocket.TcpWrite(tmp,5+bodylen);

	unsigned char szIn[1024];
	int iReadLen = stTcpCltSocket.TcpRead(szIn, sizeof(szIn));
	if (iReadLen<=0)
	{
		iBuffLen = 0;
	}
	else {
		iBuffLen = 1;
	}
	
	delete[] tmp;
	delete[] msgbody;
			
	return iBuffLen;

}


int CUserFunc::PhpPress(string arr)
{
	int iBuffLen = 0;
	char *tmp = new char[1024];
	//char *msgbody = new char[256];
	memset(tmp , 0, sizeof(tmp));
	//memset(msgbody , 0, sizeof(msgbody));
	
	sprintf(tmp,"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nCache-Control: max-age=0\r\n\r\n\n",
	"/index.php?bid=10001&did=352246067673437_460010611611439&seq=1088&delete=0&pushStat=1","test.webdev.com");
	
	TcpCltSocket stTcpCltSocket;
	char DestIp[16];
	int iDestPort = 80;
	sprintf(DestIp,"10.193.17.14");
	if(!stTcpCltSocket.ConnectServer(inet_addr(DestIp), iDestPort))
	{
		printf("connect %s:%d failed!\n",DestIp,iDestPort);
		return -1;
	}
	
	stTcpCltSocket.TcpWrite(tmp,strlen(tmp));

	unsigned char szIn[1024];
	int iReadLen = stTcpCltSocket.TcpRead(szIn, sizeof(szIn));
	if (iReadLen<=0)
	{
		iBuffLen = 0;
	}
	else {
		iBuffLen = 1;
	}
	
	delete[] tmp;
	//delete[] msgbody;
			
	return iBuffLen;

}

