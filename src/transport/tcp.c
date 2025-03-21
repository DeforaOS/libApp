/* $Id$ */
/* Copyright (c) 2012-2022 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS System libApp */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/* FIXME:
 * - added status reports as message callbacks (connecting, connected...) */



#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <limits.h>
#include <errno.h>
#ifdef __WIN32__
# include <Winsock2.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif
#include <System.h>
#include "App/appmessage.h"
#include "App/apptransport.h"

/* portability */
#ifdef __WIN32__
# define close(fd) closesocket(fd)
#endif

/* for tcp4 and tcp6 */
#ifndef TCP_DOMAIN
# define TCP_DOMAIN AF_UNSPEC
#endif


/* TCP */
/* private */
/* types */
typedef struct _AppTransportPlugin TCP;

typedef struct _TCPSocket
{
	TCP * tcp;
	AppTransportClient * client;

	int fd;
	struct sockaddr * sa;
	socklen_t sa_len;

	/* input queue */
	char * bufin;
	size_t bufin_cnt;
	/* output queue */
	char * bufout;
	size_t bufout_cnt;
} TCPSocket;

struct _AppTransportPlugin
{
	AppTransportPluginHelper * helper;
	AppTransportMode mode;

	struct addrinfo * ai;
	struct addrinfo * aip;

	union
	{
		struct
		{
			/* for servers */
			int fd;
			TCPSocket ** clients;
			size_t clients_cnt;
		} server;

		/* for clients */
		TCPSocket client;
	} u;
};


/* constants */
#define INC 1024

#include "common.h"
#include "common.c"


/* protected */
/* prototypes */
/* plug-in */
static TCP * _tcp_init(AppTransportPluginHelper * helper, AppTransportMode mode,
		char const * name);
static void _tcp_destroy(TCP * tcp);

static int _tcp_client_send(TCP * tcp, AppMessage * message);
static int _tcp_server_send(TCP * tcp, AppTransportClient * client,
		AppMessage * message);

/* useful */
static int _tcp_error(char const * message);

/* servers */
static int _tcp_server_add_client(TCP * tcp, TCPSocket * client);

/* sockets */
static int _tcp_socket_init(TCPSocket * tcpsocket, int domain, int flags,
		TCP * tcp);
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd,
		struct sockaddr * sa, socklen_t sa_len);
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_len);
static void _tcp_socket_delete(TCPSocket * tcpsocket);
static void _tcp_socket_destroy(TCPSocket * tcpsocket);

static int _tcp_socket_queue(TCPSocket * tcpsocket, Buffer * buffer);

/* callbacks */
static int _tcp_callback_accept(int fd, TCP * tcp);
static int _tcp_callback_connect(int fd, TCP * tcp);
static int _tcp_socket_callback_read(int fd, TCPSocket * tcpsocket);
static int _tcp_socket_callback_write(int fd, TCPSocket * tcpsocket);


/* public */
/* constants */
/* plug-in */
AppTransportPluginDefinition transport =
{
	"TCP",
#ifndef TRANSPORT_DESCRIPTION
# define TRANSPORT_DESCRIPTION	"Plain TCP/IP"
#endif
	TRANSPORT_DESCRIPTION,
	_tcp_init,
	_tcp_destroy,
	_tcp_client_send,
	_tcp_server_send
};


/* protected */
/* functions */
/* plug-in */
/* tcp_init */
static int _init_client(TCP * tcp, char const * name, int domain);
static int _init_server(TCP * tcp, char const * name, int domain);

static TCP * _tcp_init(AppTransportPluginHelper * helper, AppTransportMode mode,
		char const * name)
{
	TCP * tcp;
	int res;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, \"%s\")\n", __func__, mode, name);
#endif
	if((tcp = object_new(sizeof(*tcp))) == NULL)
		return NULL;
	memset(tcp, 0, sizeof(*tcp));
	tcp->helper = helper;
	switch((tcp->mode = mode))
	{
		case ATM_CLIENT:
			res = _init_client(tcp, name, TCP_DOMAIN);
			break;
		case ATM_SERVER:
			res = _init_server(tcp, name, TCP_DOMAIN);
			break;
		default:
			res = -error_set_code(-EINVAL, "%s",
					"Unknown transport mode");
			break;
	}
	/* check for errors */
	if(res != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() => %d (%s)\n", __func__, res,
				error_get(NULL));
#endif
		_tcp_destroy(tcp);
		return NULL;
	}
	return tcp;
}

static int _init_client(TCP * tcp, char const * name, int domain)
{
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	tcp->u.client.tcp = tcp;
	tcp->u.client.fd = -1;
	/* obtain the remote address */
	if((tcp->ai = _init_address(name, domain, 0)) == NULL)
		return -1;
	/* connect to the remote host */
	for(tcp->aip = tcp->ai; tcp->aip != NULL; tcp->aip = tcp->aip->ai_next)
	{
		tcp->u.client.fd = -1;
		/* initialize the client socket */
		if(_tcp_socket_init(&tcp->u.client, tcp->aip->ai_family,
					O_NONBLOCK, tcp) != 0)
			continue;
#ifdef DEBUG
		if(tcp->aip->ai_family == AF_INET)
		{
			sa = (struct sockaddr_in *)tcp->aip->ai_addr;
			fprintf(stderr, "DEBUG: %s() connect(%s:%u)\n", __func__,
					inet_ntoa(sa->sin_addr),
					ntohs(sa->sin_port));
		}
		else
			fprintf(stderr, "DEBUG: %s() connect(%d)\n", __func__,
					tcp->aip->ai_family);
#endif
		if(connect(tcp->u.client.fd, tcp->aip->ai_addr,
					tcp->aip->ai_addrlen) != 0)
		{
			if(errno != EINPROGRESS)
			{
				close(tcp->u.client.fd);
				tcp->u.client.fd = -1;
				_tcp_error("socket");
				continue;
			}
			event_register_io_write(tcp->helper->event,
					tcp->u.client.fd,
					(EventIOFunc)_tcp_callback_connect,
					tcp);
			event_loop(tcp->helper->event);
		}
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() connect() => %d\n", __func__,
				tcp->u.client.fd);
#endif
		if(tcp->u.client.fd >= 0)
			break;
	}
	if(tcp->aip == NULL)
		return -1;
	/* listen for any incoming message */
	event_register_io_read(tcp->helper->event, tcp->u.client.fd,
			(EventIOFunc)_tcp_socket_callback_read, &tcp->u.client);
	/* write pending messages if any */
	if(tcp->u.client.bufout_cnt > 0)
	{
		event_register_io_write(tcp->helper->event, tcp->u.client.fd,
				(EventIOFunc)_tcp_socket_callback_write,
				&tcp->u.client);
		event_loop(tcp->helper->event);
	}
	return 0;
}

static int _init_server(TCP * tcp, char const * name, int domain)
{
	TCPSocket tcpsocket;
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	tcp->u.server.fd = -1;
	/* obtain the local address */
	if((tcp->ai = _init_address(name, domain, AI_PASSIVE)) == NULL)
		return -1;
	for(tcp->aip = tcp->ai; tcp->aip != NULL; tcp->aip = tcp->aip->ai_next)
	{
		/* create the socket */
		if(_tcp_socket_init(&tcpsocket, tcp->aip->ai_family, O_NONBLOCK,
					tcp) != 0)
			continue;
		/* XXX ugly */
		tcp->u.server.fd = tcpsocket.fd;
		/* accept incoming connections */
#ifdef DEBUG
		if(tcp->aip->ai_family == AF_INET)
		{
			sa = (struct sockaddr_in *)tcp->aip->ai_addr;
			fprintf(stderr, "DEBUG: %s() %s (%s:%u)\n", __func__,
					"bind()", inet_ntoa(sa->sin_addr),
					ntohs(sa->sin_port));
		}
		else
			fprintf(stderr, "DEBUG: %s() %s %d\n", __func__,
					"bind()", tcp->aip->ai_family);
#endif
		if(bind(tcp->u.server.fd, tcp->aip->ai_addr,
					tcp->aip->ai_addrlen) != 0)
		{
			_tcp_error("bind");
			close(tcp->u.server.fd);
			tcp->u.server.fd = -1;
			continue;
		}
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, "listen()");
#endif
		if(listen(tcp->u.server.fd, SOMAXCONN) != 0)
		{
			_tcp_error("listen");
			close(tcp->u.server.fd);
			tcp->u.server.fd = -1;
			continue;
		}
		event_register_io_read(tcp->helper->event, tcp->u.server.fd,
				(EventIOFunc)_tcp_callback_accept, tcp);
		break;
	}
	return (tcp->aip != NULL) ? 0 : -1;
}


/* tcp_destroy */
static void _destroy_client(TCP * tcp);
static void _destroy_server(TCP * tcp);

static void _tcp_destroy(TCP * tcp)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	switch(tcp->mode)
	{
		case ATM_CLIENT:
			_destroy_client(tcp);
			break;
		case ATM_SERVER:
			_destroy_server(tcp);
			break;
	}
	if(tcp->ai != NULL)
		freeaddrinfo(tcp->ai);
	object_delete(tcp);
}

static void _destroy_client(TCP * tcp)
{
	_tcp_socket_destroy(&tcp->u.client);
}

static void _destroy_server(TCP * tcp)
{
	size_t i;

	for(i = 0; i < tcp->u.server.clients_cnt; i++)
		_tcp_socket_delete(tcp->u.server.clients[i]);
	free(tcp->u.server.clients);
	if(tcp->u.server.fd >= 0)
		close(tcp->u.server.fd);
}


/* tcp_client_send */
static int _tcp_client_send(TCP * tcp, AppMessage * message)
{
	int ret;
	Buffer * buffer;

	if(tcp->mode != ATM_CLIENT)
		return -error_set_code(1, "%s", "Not a client");
	/* send the message */
	if((buffer = buffer_new(0, NULL)) == NULL)
		return -1;
	if((ret = appmessage_serialize(message, buffer)) == 0
			&& (ret = _tcp_socket_queue(&tcp->u.client, buffer)) == 0)
		event_loop(tcp->helper->event);
	buffer_delete(buffer);
	return ret;
}


/* tcp_server_send */
static int _tcp_server_send(TCP * tcp, AppTransportClient * client,
		AppMessage * message)
{
	size_t i;
	TCPSocket * s;
	Buffer * buffer;

	if(tcp->mode != ATM_SERVER)
		return -error_set_code(1, "%s", "Not a server");
	/* lookup the client */
	for(i = 0; i < tcp->u.server.clients_cnt; i++)
	{
		s = tcp->u.server.clients[i];
		if(s->client == client)
			break;
	}
	if(i == tcp->u.server.clients_cnt)
		return -error_set_code(1, "%s", "Unknown client");
	/* send the message */
	if((buffer = buffer_new(0, NULL)) == NULL)
		return -1;
	if(appmessage_serialize(message, buffer) != 0
			|| _tcp_socket_queue(s, buffer) != 0)
	{
		buffer_delete(buffer);
		return -1;
	}
	return 0;
}


/* useful */
/* tcp_error */
static int _tcp_error(char const * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, message);
#endif
	return error_set_code(-errno, "%s%s%s",
			(message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
}


/* servers */
/* tcp_server_add_client */
static int _tcp_server_add_client(TCP * tcp, TCPSocket * client)
{
	TCPSocket ** p;
#ifndef NI_MAXHOST
# define NI_MAXHOST 256
#endif
	char host[NI_MAXHOST];
	char const * name = host;
	const int flags = NI_NUMERICSERV;

	if((p = realloc(tcp->u.server.clients, sizeof(*p)
					* (tcp->u.server.clients_cnt + 1)))
			== NULL)
		return -1;
	tcp->u.server.clients = p;
	/* XXX may not be instant */
	if(getnameinfo(client->sa, client->sa_len, host, sizeof(host), NULL, 0,
				NI_NAMEREQD | flags) != 0
			&& getnameinfo(client->sa, client->sa_len, host,
				sizeof(host), NULL, 0, NI_NUMERICHOST | flags)
			!= 0)
		name = NULL;
	if((client->client = tcp->helper->client_new(tcp->helper->transport,
					name)) == NULL)
		return -1;
	tcp->u.server.clients[tcp->u.server.clients_cnt++] = client;
	return 0;
}


/* sockets */
/* tcp_socket_init */
static int _tcp_socket_init(TCPSocket * tcpsocket, int domain, int flags,
		TCP * tcp)
{
	int f;

	if((tcpsocket->fd = socket(domain, SOCK_STREAM, 0)) < 0)
		return -_tcp_error("socket");
	_tcp_socket_init_fd(tcpsocket, tcp, tcpsocket->fd, NULL, 0);
	/* set the socket flags */
	if(flags != 0)
	{
		if((f = fcntl(tcpsocket->fd, F_GETFL)) == -1)
			return -_tcp_error("fcntl");
		if((f & flags) != flags)
			if(fcntl(tcpsocket->fd, F_SETFL, f | flags) == -1)
				return -_tcp_error("fcntl");
	}
#ifdef TCP_NODELAY
	/* do not wait before sending any traffic */
	f = 1;
	setsockopt(fd, SOL_SOCKET, TCP_NODELAY, &f, sizeof(f));
#endif
	return 0;
}


/* tcp_socket_init_fd */
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd,
		struct sockaddr * sa, socklen_t sa_len)
{
	tcpsocket->tcp = tcp;
	tcpsocket->client = NULL;
	tcpsocket->fd = fd;
	tcpsocket->sa = sa;
	tcpsocket->sa_len = sa_len;
	tcpsocket->bufin = NULL;
	tcpsocket->bufin_cnt = 0;
	tcpsocket->bufout = NULL;
	tcpsocket->bufout_cnt = 0;
}


/* tcp_socket_new_fd */
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_len)
{
	TCPSocket * tcpsocket;

	if((tcpsocket = object_new(sizeof(*tcpsocket))) == NULL)
		return NULL;
	_tcp_socket_init_fd(tcpsocket, tcp, fd, sa, sa_len);
	return tcpsocket;
}


/* tcp_socket_delete */
static void _tcp_socket_delete(TCPSocket * tcpsocket)
{
	_tcp_socket_destroy(tcpsocket);
	object_delete(tcpsocket);
}


/* tcp_socket_destroy */
static void _tcp_socket_destroy(TCPSocket * tcpsocket)
{
	TCP * tcp = tcpsocket->tcp;
	AppTransportPluginHelper * helper = tcp->helper;

	helper->client_delete(helper->transport, tcpsocket->client);
	free(tcpsocket->sa);
	if(tcpsocket->fd >= 0)
	{
		event_unregister_io_read(tcpsocket->tcp->helper->event,
				tcpsocket->fd);
		event_unregister_io_write(tcpsocket->tcp->helper->event,
				tcpsocket->fd);
		close(tcpsocket->fd);
	}
	free(tcpsocket->bufin);
	free(tcpsocket->bufout);
}


/* tcp_socket_queue */
static int _tcp_socket_queue(TCPSocket * tcpsocket, Buffer * buffer)
{
	uint32_t len;
	char * p;
	Variable * v;
	Buffer * b = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, tcpsocket->fd);
#endif
	/* serialize the buffer */
	v = variable_new(VT_BUFFER, buffer);
	b = buffer_new(0, NULL);
	if(v == NULL || b == NULL || variable_serialize(v, b, 0) != 0)
	{
		if(v != NULL)
			variable_delete(v);
		if(b != NULL)
			buffer_delete(b);
		return -1;
	}
	variable_delete(v);
	len = buffer_get_size(b);
	/* FIXME queue the serialized buffer directly as a message instead */
	if((p = realloc(tcpsocket->bufout, tcpsocket->bufout_cnt + len))
			== NULL)
	{
		buffer_delete(b);
		return -1;
	}
	tcpsocket->bufout = p;
	memcpy(&p[tcpsocket->bufout_cnt], buffer_get_data(b), len);
	/* register the callback if necessary */
	if(tcpsocket->bufout_cnt == 0)
		event_register_io_write(tcpsocket->tcp->helper->event,
				tcpsocket->fd,
				(EventIOFunc)_tcp_socket_callback_write,
				tcpsocket);
	tcpsocket->bufout_cnt += len;
	buffer_delete(b);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d) => %d\n", __func__, tcpsocket->fd, 0);
#endif
	return 0;
}


/* callbacks */
/* tcp_callback_accept */
static int _accept_client(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_len);

static int _tcp_callback_accept(int fd, TCP * tcp)
{
	struct sockaddr * sa;
	socklen_t sa_len = tcp->aip->ai_addrlen;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcp->u.server.fd != fd)
		return -1;
	if((sa = malloc(sa_len)) == NULL)
		/* XXX this may not be enough to recover */
		sa_len = 0;
	if((fd = accept(fd, sa, &sa_len)) < 0)
	{
		free(sa);
		return _tcp_error("accept");
	}
	if(_accept_client(tcp, fd, sa, sa_len) != 0)
	{
		/* just close the connection and keep serving */
		/* FIXME report error */
		close(fd);
		free(sa);
	}
#ifdef DEBUG
	else
		fprintf(stderr, "DEBUG: %s() %d\n", __func__, fd);
#endif
	return 0;
}

static int _accept_client(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_len)
{
	TCPSocket * tcpsocket;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	if((tcpsocket = _tcp_socket_new_fd(tcp, fd, sa, sa_len)) == NULL)
		return -1;
	if(_tcp_server_add_client(tcp, tcpsocket) != 0)
	{
		/* XXX workaround for a double-close() */
		tcpsocket->fd = -1;
		_tcp_socket_delete(tcpsocket);
		return -1;
	}
	event_register_io_read(tcp->helper->event, tcpsocket->fd,
			(EventIOFunc)_tcp_socket_callback_read, tcpsocket);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d) => 0\n", __func__, fd);
#endif
	return 0;
}


/* tcp_callback_connect */
static int _tcp_callback_connect(int fd, TCP * tcp)
{
	int ret;
	socklen_t s = sizeof(ret);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcp->u.client.fd != fd)
		return -1;
	/* obtain the connection status */
	if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &ret, &s) != 0)
	{
		error_set_code(-errno, "%s: %s", "getsockopt", strerror(errno));
		close(fd);
		tcp->u.client.fd = -1;
		/* FIXME report error */
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, strerror(errno));
#endif
		ret = -1;
	}
	else if(ret != 0)
	{
		/* the connection failed */
		error_set_code(-ret, "%s: %s", "connect", strerror(ret));
		close(fd);
		tcp->u.client.fd = -1;
		/* FIXME report error */
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, strerror(ret));
#endif
		ret = -1;
	}
	else
		/* deregister this callback */
		ret = 1;
	if(tcp->mode == ATM_CLIENT)
		event_loop_quit(tcp->helper->event);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() => %d\n", __func__, ret);
#endif
	return ret;
}


/* tcp_socket_callback_read */
static AppMessage * _socket_callback_message(TCPSocket * tcpsocket);
static void _socket_callback_read_client(TCPSocket * tcpsocket,
		AppMessage * message);
static void _socket_callback_read_server(TCPSocket * tcpsocket,
		AppMessage * message);
static int _socket_callback_recv(TCPSocket * tcpsocket);

static int _tcp_socket_callback_read(int fd, TCPSocket * tcpsocket)
{
	AppMessage * message;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcpsocket->fd != fd)
		return -1;
	if(_socket_callback_recv(tcpsocket) != 0)
		return -1;
	while((message = _socket_callback_message(tcpsocket)) != NULL)
	{
		switch(tcpsocket->tcp->mode)
		{
			case ATM_CLIENT:
				_socket_callback_read_client(tcpsocket,
						message);
				break;
			case ATM_SERVER:
				_socket_callback_read_server(tcpsocket,
						message);
				break;
		}
		appmessage_delete(message);
	}
	return 0;
}

static AppMessage * _socket_callback_message(TCPSocket * tcpsocket)
{
	AppMessage * message = NULL;
	size_t size;
	Variable * variable;
	Buffer * buffer;

	size = tcpsocket->bufin_cnt;
	/* deserialize the data as a buffer (containing a message) */
	if((variable = variable_new_deserialize_type(VT_BUFFER, &size,
					tcpsocket->bufin)) == NULL)
		/* XXX assumes not enough data was available */
		return NULL;
	tcpsocket->bufin_cnt -= size;
	memmove(tcpsocket->bufin, &tcpsocket->bufin[size],
			tcpsocket->bufin_cnt);
	if((variable_get_as(variable, VT_BUFFER, &buffer, NULL)) == 0)
	{
		message = appmessage_new_deserialize(buffer);
		buffer_delete(buffer);
	}
	variable_delete(variable);
	return message;
}

static void _socket_callback_read_client(TCPSocket * tcpsocket,
		AppMessage * message)
{
	AppTransportPluginHelper * helper = tcpsocket->tcp->helper;

	helper->receive(helper->transport, message);
}

static void _socket_callback_read_server(TCPSocket * tcpsocket,
		AppMessage * message)
{
	AppTransportPluginHelper * helper = tcpsocket->tcp->helper;

	helper->client_receive(helper->transport, tcpsocket->client,
			message);
}

static int _socket_callback_recv(TCPSocket * tcpsocket)
{
	const size_t inc = INC;
	ssize_t ssize;
	char * p;

	if((p = realloc(tcpsocket->bufin, tcpsocket->bufin_cnt + inc)) == NULL)
		return -1;
	tcpsocket->bufin = p;
	if((ssize = recv(tcpsocket->fd, &tcpsocket->bufin[tcpsocket->bufin_cnt],
					inc, 0)) < 0)
	{
		error_set_code(-errno, "%s", strerror(errno));
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* FIXME report error */
		return -1;
	}
	else if(ssize == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() recv() => %ld\n", __func__, ssize);
#endif
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* FIXME report transfer clean shutdown */
		return -1;
	}
	tcpsocket->bufin_cnt += ssize;
	return 0;
}


/* tcp_socket_callback_write */
static int _tcp_socket_callback_write(int fd, TCPSocket * tcpsocket)
{
	ssize_t ssize;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcpsocket->fd != fd)
		return -1;
	if((ssize = send(tcpsocket->fd, tcpsocket->bufout,
					tcpsocket->bufout_cnt, 0)) < 0)
	{
		/* XXX report error (and reconnect) */
		error_set_code(-errno, "%s", strerror(errno));
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		return -1;
	}
	else if(ssize == 0)
	{
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* XXX report transfer interruption (and reconnect) */
		return -error_set_code(-errno, "%s", strerror(errno));
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() send() => %ld\n", __func__, ssize);
#endif
	/* XXX use a sliding cursor instead (and then queue the next message) */
	memmove(tcpsocket->bufout, &tcpsocket->bufout[ssize],
			tcpsocket->bufout_cnt - ssize);
	tcpsocket->bufout_cnt -= ssize;
	/* unregister the callback if there is nothing left to write */
	if(tcpsocket->bufout_cnt == 0)
	{
		if(tcpsocket->tcp->mode == ATM_CLIENT)
			event_loop_quit(tcpsocket->tcp->helper->event);
		return 1;
	}
	return 0;
}
