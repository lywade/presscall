#include "MainSoWork.hpp"



struct cgi_evt_para_t
{
	MainSoWork* m_so; 

	struct event_base* evt_base;
	struct event* evt;

	enum sock_type sock_type;
};


pthread_mutex_t g_job_mtx = PTHREAD_MUTEX_INITIALIZER; 


void cgi_evt_hdl(int fd, short ev, void* arg);

int accept_connect(int listen_fd, struct event_base* evt_base, MainSoWork* m_so)
{
	int cli_fd;
	struct sockaddr_in cli_addr;
	socklen_t cli_len = sizeof(cli_addr);

	cli_fd = accept(listen_fd,
			(struct sockaddr *)&cli_addr,
			&cli_len);
	if (cli_fd < 0)
	{
		ERR("cli: accept() failed:%s", strerror(errno));
		return -1;
	} 

	struct timeval tm; tm.tv_sec = 5; tm.tv_usec = 0;
	if (setsockopt(cli_fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tm, sizeof(tm)) < 0    
		|| setsockopt(cli_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tm, sizeof(tm)) < 0)    
	{        
		ERR("setsockopt: %s\n", strerror(errno));        
		return -1;
	}

	struct cgi_evt_para_t* cgi_evt_para = new(struct cgi_evt_para_t); 
	cgi_evt_para->m_so = m_so;
	cgi_evt_para->sock_type = cli_sock;

	struct event* cli_evt = event_new(evt_base, cli_fd, 
			EV_READ | EV_PERSIST, cgi_evt_hdl, 
			cgi_evt_para); 
	assert(cli_evt);

	cgi_evt_para->evt = cli_evt;

	event_add(cli_evt, NULL);

	return 0; 
}


int send_msg(char* buf, int buf_len, int fd, struct event* evt, MainSoWork* m_so)
{
	int ret;	
	int whole_sent = 0;

	do
	{
		ret = write(fd, buf, buf_len);
		if(ret < 0) 
		{
			if(errno != EAGAIN)
			{
				close(fd);
				event_del(evt);
				event_free(evt); 
				ERR("write failed:%s\n", strerror(errno));
				return -1;
			}
			else
			{ 
				continue;
			}
		}
		whole_sent += ret;
	}while(whole_sent < buf_len);

	return 0; 
}


int memcached_complete_func(const void* data, unsigned int data_len, 
		int *i_pkg_theory_len);
int read_cli_req(int fd, struct event* evt, MainSoWork* m_so)
{
	char buf[4096] = {0};
	unsigned to_read_len = sizeof(buf) - 1;
	unsigned read_len = 0;
	int theory_len;
	int ret;

	do
	{
		ret = read(fd, buf, to_read_len - read_len);
		if(ret == 0)
		{
			//WARN("peer socket closed\n");
			break;
		} 
		if(ret < 0)
		{
			if(errno != EAGAIN)
			{ 
				ERR("read error:%s\n", strerror(errno));
				break;
			} 
		}
		read_len += ret;

	}while(memcached_complete_func(buf, read_len, &theory_len) <= 0);

	if(ret <= 0)
	{
		close(fd);
		event_del(evt);
		event_free(evt); 

		//WARN("evt freed\n");

		return -1;
	}

	pthread_mutex_lock(&g_job_mtx);

	m_so->process_cgi_msg(buf, theory_len, fd, evt, send_msg);	

	pthread_mutex_unlock(&g_job_mtx);

	return 0;
}


void cgi_evt_hdl(int fd, short ev, void* arg)
{
	struct cgi_evt_para_t* cgi_evt_para
		= (struct cgi_evt_para_t*)arg;

	MainSoWork* m_so = (MainSoWork*)cgi_evt_para->m_so;

	switch(cgi_evt_para->sock_type) 
	{
		case listen_sock:
			accept_connect(fd, cgi_evt_para->evt_base, m_so);
			return;
		case cli_sock:
			if(read_cli_req(fd, cgi_evt_para->evt, cgi_evt_para->m_so))
			{
				delete cgi_evt_para;
			}
			return;
	} 
}


void* event_thread_func(void* para)
{
	MainSoWork* m_so = (MainSoWork*)para; 

	int fd;
	int ret;
	ret = create_listen_sock("0.0.0.0", m_so->m_conf.local_listen_cgi_port, &fd);
	assert(ret == 0);

	struct event_base* evt_base = m_so->m_evt_base;

	struct cgi_evt_para_t cgi_evt_para; 
	cgi_evt_para.m_so = m_so;
	cgi_evt_para.sock_type = listen_sock;

	struct event* accept_evt = event_new(evt_base, fd, 
			EV_READ | EV_PERSIST, cgi_evt_hdl, &cgi_evt_para); 
	assert(accept_evt);

	cgi_evt_para.evt = accept_evt;
	cgi_evt_para.evt_base = evt_base;

	event_add(accept_evt, NULL);

	event_base_dispatch(evt_base); 

	event_free(accept_evt);
	event_base_free(evt_base); 

	return NULL;
}


int create_listen_sock(char* ip, int port, int* fd)
{
	struct sockaddr_in local_addr;

	memset(&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	local_addr.sin_addr.s_addr = inet_addr(ip);

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0)
	{
		fprintf(stderr, "socket: %s\n", strerror(errno));
		exit(-1);
	}

	int flag = 1;
	if(setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)))
	{
		fprintf(stderr, "setsocketopt: %s\n", strerror(errno));        
		close(sfd);
		exit(-1);
	}

	flag = 1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag)))
	{
		fprintf(stderr, "setsocketopt: %s\n", strerror(errno));        
		close(sfd);
		exit(-1);
	}

	socklen_t addr_len = sizeof(local_addr); 
	if (bind(sfd, (const sockaddr*)&local_addr, addr_len) == -1) 
	{
		fprintf(stderr, "bind: %s\n", strerror(errno));        
		close(sfd);
		exit(-1);
	}

	if(listen(sfd, 100))
	{
		fprintf(stderr, "listen: %s\n", strerror(errno));        
		close(sfd);
		exit(-1);
	}

	*fd = sfd;

	return 0;
}

