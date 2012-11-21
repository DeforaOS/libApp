/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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
#include "App/apptransport.h"

/* portability */
#ifdef __WIN32__
# define close(fd) closesocket(fd)
#endif

/* for tcp4 and tcp6 */
#ifndef TCP_FAMILY
# define TCP_FAMILY AF_INET
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


/* protected */
/* prototypes */
/* plug-in */
static TCP * _tcp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name);
static void _tcp_destroy(TCP * tcp);

static int _tcp_send(TCP * tcp, AppMessage * message, int acknowledge);

/* useful */
static int _tcp_error(char const * message, int code);

/* servers */
static int _tcp_server_add_client(TCP * tcp, TCPSocket * client);

/* sockets */
static int _tcp_socket_init(TCPSocket * tcpsocket, int domain, TCP * tcp);
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd);
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd);
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
	NULL,
	_tcp_init,
	_tcp_destroy,
	_tcp_send
};


/* protected */
/* functions */
/* plug-in */
/* tcp_init */
static int _init_address(char const * name, int * domain,
		struct sockaddr_in * sa);
static int _init_client(TCP * tcp, char const * name);
static int _init_server(TCP * tcp, char const * name);

static TCP * _tcp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name)
{
	TCP * tcp;
	int res;

	if((tcp = object_new(sizeof(*tcp))) == NULL)
		return NULL;
	memset(tcp, 0, sizeof(*tcp));
	tcp->helper = helper;
	switch((tcp->mode = mode))
	{
		case ATM_CLIENT:
			res = _init_client(tcp, name);
			break;
		case ATM_SERVER:
			res = _init_server(tcp, name);
			break;
		default:
			res = -error_set_code(1, "Unknown transport mode");
			break;
	}
	/* check for errors */
	if(res != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() => %d\n", __func__, res);
#endif
		_tcp_destroy(tcp);
		return NULL;
	}
	return tcp;
}

static int _init_address(char const * name, int * domain,
		struct sockaddr_in * sa)
{
	char * p;
	char * q;
	char * r;
	struct hostent * he;
	long l = -1;

	/* obtain the name */
	if((p = strdup(name)) == NULL)
		return -error_set_code(1, "%s", strerror(errno));
	if((q = strchr(p, ':')) != NULL)
	{
		*(q++) = '\0';
		/* obtain the port number */
		l = strtol(q, &r, 10);
		if(q[0] == '\0' || *r != '\0' || l < 0 || l > 65535)
			l = -error_set_code(1, "%s", strerror(EINVAL));
	}
	/* FIXME perform this asynchronously */
	if((he = gethostbyname(p)) == NULL)
		l = -error_set_code(1, "%s", hstrerror(h_errno));
	free(p);
	/* check for errors */
	if(l < 0)
		return -1;
	sa->sin_family = *domain;
	sa->sin_port = htons(l);
	memcpy(&sa->sin_addr, he->h_addr_list[0], sizeof(sa->sin_addr));
	return 0;
}

static int _init_client(TCP * tcp, char const * name)
{
	int domain = TCP_FAMILY;
	struct sockaddr_in sa;

	/* obtain the remote address */
	if(_init_address(name, &domain, &sa) != 0)
		return -1;
	/* initialize the client socket */
	if(_tcp_socket_init(&tcp->u.client, domain, tcp) != 0)
		return -1;
	/* connect to the remote host */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %s (%s:%u)\n", __func__, "connect()",
			inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
#endif
	if(connect(tcp->u.client.fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
	{
		if(errno != EINPROGRESS)
			return -_tcp_error("socket", 1);
		else
			event_register_io_write(tcp->helper->event,
					tcp->u.client.fd,
					(EventIOFunc)_tcp_callback_connect,
					tcp);
	}
	else
		event_register_io_read(tcp->helper->event, tcp->u.client.fd,
				(EventIOFunc)_tcp_socket_callback_read,
				&tcp->u.client);
	return 0;
}

static int _init_server(TCP * tcp, char const * name)
{
	TCPSocket tcpsocket;
	int domain = TCP_FAMILY;
	struct sockaddr_in sa;

	/* obtain the local address */
	if(_init_address(name, &domain, &sa) != 0)
		return -1;
	/* create the socket */
	if(_tcp_socket_init(&tcpsocket, domain, tcp) != 0)
		return -1;
	/* XXX ugly */
	tcp->u.server.fd = tcpsocket.fd;
	/* accept incoming connections */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %s (%s:%u)\n", __func__, "bind()",
			inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
#endif
	if(bind(tcp->u.server.fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
		return -_tcp_error("bind", 1);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %s\n", __func__, "listen()");
#endif
	if(listen(tcp->u.server.fd, SOMAXCONN) != 0)
		return -_tcp_error("listen", 1);
	event_register_io_read(tcp->helper->event, tcp->u.server.fd,
			(EventIOFunc)_tcp_callback_accept, tcp);
	return 0;
}


/* tcp_destroy */
static void _tcp_destroy(TCP * tcp)
{
	switch(tcp->mode)
	{
		case ATM_CLIENT:
			_tcp_socket_destroy(&tcp->u.client);
			break;
		case ATM_SERVER:
			if(tcp->u.server.fd >= 0)
				close(tcp->u.server.fd);
			break;
	}
	object_delete(tcp);
}


/* tcp_send */
static int _tcp_send(TCP * tcp, AppMessage * message, int acknowledge)
{
	/* FIXME implement */
	return -1;
}


/* useful */
/* tcp_error */
static int _tcp_error(char const * message, int code)
{
	return error_set_code(code, "%s%s%s", (message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
}


/* servers */
/* tcp_server_add_client */
static int _tcp_server_add_client(TCP * tcp, TCPSocket * client)
{
	TCPSocket ** p;

	if((p = realloc(tcp->u.server.clients, sizeof(*p)
					* (tcp->u.server.clients_cnt + 1)))
			== NULL)
		return -1;
	tcp->u.server.clients = p;
	if((client->client = tcp->helper->client_new(tcp->helper->transport))
			== NULL)
		return -1;
	tcp->u.server.clients[tcp->u.server.clients_cnt++] = client;
	return 0;
}


/* sockets */
/* tcp_socket_init */
static int _tcp_socket_init(TCPSocket * tcpsocket, int domain, TCP * tcp)
{
	int flags;

	if((tcpsocket->fd = socket(domain, SOCK_STREAM, 0)) < 0)
		return -_tcp_error("socket", 1);
	_tcp_socket_init_fd(tcpsocket, tcp, tcpsocket->fd);
	/* set the socket as non-blocking */
	if((flags = fcntl(tcpsocket->fd, F_GETFL)) == -1)
		return -_tcp_error("fcntl", 1);
	if((flags & O_NONBLOCK) == 0)
		if(fcntl(tcpsocket->fd, F_SETFL, flags | O_NONBLOCK) == -1)
			return -_tcp_error("fcntl", 1);
	return 0;
}


/* tcp_socket_init_fd */
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd)
{
	tcpsocket->tcp = tcp;
	tcpsocket->client = NULL;
	tcpsocket->fd = fd;
	tcpsocket->bufin = NULL;
	tcpsocket->bufin_cnt = 0;
	tcpsocket->bufout = NULL;
	tcpsocket->bufout_cnt = 0;
}


/* tcp_socket_new_fd */
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd)
{
	TCPSocket * tcpsocket;

	if((tcpsocket = object_new(sizeof(*tcpsocket))) == NULL)
		return NULL;
	_tcp_socket_init_fd(tcpsocket, tcp, fd);
	return tcpsocket;
}


/* tcp_socket_queue */
static int _tcp_socket_queue(TCPSocket * tcpsocket, Buffer * buffer)
{
	size_t inc;
	uint32_t len;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, tcpsocket->fd);
#endif
	/* FIXME serialize the buffer instead */
	len = buffer_get_size(buffer);
	if((inc = ((len + sizeof(len)) | (INC - 1)) + 1) < len + sizeof(len))
		inc = len + sizeof(len);
	if((p = realloc(tcpsocket->bufout, tcpsocket->bufout_cnt + inc))
			== NULL)
		return -1;
	tcpsocket->bufout = p;
	/* FIXME use the proper endian */
	memcpy(tcpsocket->bufout, &len, sizeof(len));
	memcpy(&tcpsocket->bufout[sizeof(len)], buffer_get_data(buffer), len);
	/* register the callback if necessary */
	if(tcpsocket->bufout_cnt == 0)
		event_register_io_write(tcpsocket->tcp->helper->event,
				tcpsocket->fd,
				(EventIOFunc)_tcp_socket_callback_write,
				tcpsocket);
	tcpsocket->bufout_cnt += len + sizeof(len);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d) => 0\n", __func__, tcpsocket->fd);
#endif
	return 0;
}


/* callbacks */
/* tcp_callback_accept */
static int _accept_client(TCP * tcp, int fd);

static int _tcp_callback_accept(int fd, TCP * tcp)
{
	struct sockaddr_in sa;
	socklen_t sa_size = sizeof(sa);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcp->u.server.fd != fd)
		return -1;
	if((fd = accept(fd, (struct sockaddr *)&sa, &sa_size)) < 0)
		return _tcp_error("accept", 1);
	if(_accept_client(tcp, fd) != 0)
		/* just close the connection and keep serving */
		/* FIXME report error */
		close(fd);
#ifdef DEBUG
	else
		fprintf(stderr, "DEBUG: %s() %d\n", __func__, fd);
#endif
	return 0;
}

static int _accept_client(TCP * tcp, int fd)
{
	TCPSocket * tcpsocket;
	Buffer * buf;
	char const banner[4] = "App!";

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	if((tcpsocket = _tcp_socket_new_fd(tcp, fd)) == NULL)
		return -1;
	/* send the banner */
	if((buf = buffer_new(sizeof(banner), banner)) == NULL
			|| _tcp_socket_queue(tcpsocket, buf) != 0)
	{
		buffer_delete(buf);
		return -1;
	}
	buffer_delete(buf);
	if(_tcp_server_add_client(tcp, tcpsocket) != 0)
	{
		/* XXX workaround for a double-close() */
		event_unregister_io_read(tcp->helper->event, tcpsocket->fd);
		event_unregister_io_write(tcp->helper->event, tcpsocket->fd);
		tcpsocket->fd = -1;
		_tcp_socket_delete(tcpsocket);
		return -1;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d) => 0\n", __func__, fd);
#endif
	return 0;
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


/* tcp_callback_connect */
static int _tcp_callback_connect(int fd, TCP * tcp)
{
	int res;
	socklen_t s = sizeof(res);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcp->u.client.fd != fd)
		return -1;
	/* obtain the connection status */
	if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &res, &s) != 0)
	{
		error_set_code(1, "%s: %s", "getsockopt", strerror(errno));
		close(fd);
		tcp->u.client.fd = -1;
		/* FIXME report error */
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, strerror(errno));
#endif
		return -1;
	}
	if(res != 0)
	{
		/* the connection failed */
		error_set_code(1, "%s: %s", "connect", strerror(res));
		close(fd);
		tcp->u.client.fd = -1;
		/* FIXME report error */
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, strerror(res));
#endif
		return -1;
	}
	/* listen for any incoming message */
	event_register_io_read(tcp->helper->event, fd,
			(EventIOFunc)_tcp_socket_callback_read,
			&tcp->u.client);
	return 1;
}


/* tcp_socket_callback_read */
static int _tcp_socket_callback_read(int fd, TCPSocket * tcpsocket)
{
	const size_t inc = INC;
	ssize_t ssize;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcpsocket->fd != fd)
		return -1;
	if((p = realloc(tcpsocket->bufin, tcpsocket->bufin_cnt + inc)) == NULL)
		return -1;
	tcpsocket->bufin = p;
	if((ssize = recv(tcpsocket->fd, &tcpsocket->bufin[tcpsocket->bufin_cnt],
					inc, 0)) < 0)
	{
		error_set_code(1, "%s", strerror(errno));
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* FIXME report error */
		return -1;
	}
	else if(ssize == 0)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() read() => %ld\n", __func__, ssize);
#endif
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* FIXME report transfer clean shutdown */
		return -1;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() read() => %ld\n", __func__, ssize);
#endif
	/* FIXME parse the incoming data:
	 * - deserialize as a buffer (message unit)
	 * - deserialize the buffer as a message */
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
		error_set_code(1, "%s", strerror(errno));
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		return -1;
	}
	else if(ssize == 0)
	{
		close(tcpsocket->fd);
		tcpsocket->fd = -1;
		/* XXX report transfer interruption (and reconnect) */
		return -error_set_code(1, "%s", strerror(errno));
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() write() => %ld\n", __func__, ssize);
#endif
	/* XXX use a sliding cursor instead */
	memmove(tcpsocket->bufout, &tcpsocket->bufout[ssize],
			tcpsocket->bufout_cnt - ssize);
	tcpsocket->bufout_cnt -= ssize;
	/* unregister the callback if there is nothing left to write */
	if(tcpsocket->bufout_cnt == 0)
		return 1;
	return 0;
}
