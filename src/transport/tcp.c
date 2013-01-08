/* $Id$ */
/* Copyright (c) 2012-2013 Pierre Pronchery <khorben@defora.org> */
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
#ifndef TCP_FAMILY
# define TCP_FAMILY PF_UNSPEC
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
	socklen_t sa_size;

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
	socklen_t ai_addrlen;

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
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd,
		struct sockaddr * sa, socklen_t sa_size);
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_size);
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
static int _init_address(TCP * tcp, char const * name, int domain);
static int _init_client(TCP * tcp, char const * name);
static int _init_server(TCP * tcp, char const * name);

static TCP * _tcp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name)
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
		fprintf(stderr, "DEBUG: %s() => %d (%s)\n", __func__, res,
				error_get());
#endif
		_tcp_destroy(tcp);
		return NULL;
	}
	return tcp;
}

static int _init_address(TCP * tcp, char const * name, int domain)
{
	char sep = ':';
	char * p;
	char * q;
	char * r;
	long l;
	struct addrinfo hints;
	int res = 0;

	/* guess the port separator */
	if((q = strchr(name, ':')) != NULL && strchr(++q, ':') != NULL)
		sep = '.';
	/* obtain the name */
	if((p = strdup(name)) == NULL)
		return -error_set_code(1, "%s", strerror(errno));
	/* obtain the port number */
	if((q = strrchr(p, sep)) == NULL)
		l = -error_set_code(1, "%s", strerror(EINVAL));
	else
	{
		*(q++) = '\0';
		l = strtol(q, &r, 10);
		if(q[0] == '\0' || *r != '\0')
			l = -error_set_code(1, "%s", strerror(EINVAL));
	}
	/* FIXME perform this asynchronously */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = domain;
	hints.ai_socktype = SOCK_STREAM;
	if(l >= 0)
		res = getaddrinfo(p, q, &hints, &tcp->ai);
	free(p);
	/* check for errors */
	if(res != 0)
	{
		error_set_code(1, "%s", gai_strerror(res));
		if(tcp->ai != NULL)
			freeaddrinfo(tcp->ai);
		tcp->ai = NULL;
		return -1;
	}
	return (tcp->ai != NULL) ? 0 : -1;
}

static int _init_client(TCP * tcp, char const * name)
{
	struct addrinfo * aip;
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	tcp->u.client.fd = -1;
	/* obtain the remote address */
	if(_init_address(tcp, name, TCP_FAMILY) != 0)
		return -1;
	/* connect to the remote host */
	for(aip = tcp->ai; aip != NULL; aip = aip->ai_next)
	{
		tcp->u.client.fd = -1;
		/* initialize the client socket */
		if(_tcp_socket_init(&tcp->u.client, aip->ai_family, tcp) != 0)
			continue;
#ifdef DEBUG
		if(aip->ai_family == PF_INET)
		{
			sa = (struct sockaddr_in *)aip->ai_addr;
			fprintf(stderr, "DEBUG: %s() %s (%s:%u)\n", __func__,
					"connect()", inet_ntoa(sa->sin_addr),
					ntohs(sa->sin_port));
		}
		else
			fprintf(stderr, "DEBUG: %s() %s %d\n", __func__,
					"connect()", aip->ai_family);
#endif
		if(connect(tcp->u.client.fd, aip->ai_addr, aip->ai_addrlen)
				!= 0)
		{
			if(errno != EINPROGRESS)
			{
				close(tcp->u.client.fd);
				tcp->u.client.fd = -1;
				_tcp_error("socket", 1);
				continue;
			}
			event_register_io_write(tcp->helper->event,
					tcp->u.client.fd,
					(EventIOFunc)_tcp_callback_connect,
					tcp);
		}
		else
			event_register_io_read(tcp->helper->event,
					tcp->u.client.fd,
					(EventIOFunc)_tcp_socket_callback_read,
					&tcp->u.client);
		tcp->ai_addrlen = aip->ai_addrlen;
		break;
	}
	freeaddrinfo(tcp->ai);
	tcp->ai = NULL;
	return (aip != NULL) ? 0 : -1;
}

static int _init_server(TCP * tcp, char const * name)
{
	struct addrinfo * aip;
	TCPSocket tcpsocket;
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	/* obtain the local address */
	if(_init_address(tcp, name, TCP_FAMILY) != 0)
		return -1;
	for(aip = tcp->ai; aip != NULL; aip = aip->ai_next)
	{
		tcp->u.server.fd = -1;
		/* create the socket */
		if(_tcp_socket_init(&tcpsocket, aip->ai_family, tcp) != 0)
			continue;
		/* XXX ugly */
		tcp->u.server.fd = tcpsocket.fd;
		/* accept incoming connections */
#ifdef DEBUG
		if(aip->ai_family == PF_INET)
		{
			sa = (struct sockaddr_in *)aip->ai_addr;
			fprintf(stderr, "DEBUG: %s() %s (%s:%u)\n", __func__,
					"bind()", inet_ntoa(sa->sin_addr),
					ntohs(sa->sin_port));
		}
		else
			fprintf(stderr, "DEBUG: %s() %s %d\n", __func__,
					"bind()", aip->ai_family);
#endif
		if(bind(tcp->u.server.fd, aip->ai_addr, aip->ai_addrlen) != 0)
		{
			_tcp_error("bind", 1);
			close(tcp->u.server.fd);
			tcp->u.server.fd = -1;
			continue;
		}
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %s\n", __func__, "listen()");
#endif
		if(listen(tcp->u.server.fd, SOMAXCONN) != 0)
		{
			_tcp_error("listen", 1);
			close(tcp->u.server.fd);
			tcp->u.server.fd = -1;
			continue;
		}
		tcp->ai_addrlen = aip->ai_addrlen;
		event_register_io_read(tcp->helper->event, tcp->u.server.fd,
				(EventIOFunc)_tcp_callback_accept, tcp);
		break;
	}
	freeaddrinfo(tcp->ai);
	tcp->ai = NULL;
	return (aip != NULL) ? 0 : -1;
}


/* tcp_destroy */
static void _tcp_destroy(TCP * tcp)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
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
	if(tcp->ai != NULL)
		freeaddrinfo(tcp->ai);
	object_delete(tcp);
}


/* tcp_send */
static int _tcp_send(TCP * tcp, AppMessage * message, int acknowledge)
{
	Buffer * buffer;

	if(tcp->mode != ATM_CLIENT)
		return -error_set_code(1, "%s", "Not a client");
	if((buffer = buffer_new(0, NULL)) == NULL)
		return -1;
	if(appmessage_serialize(message, buffer) != 0
			|| _tcp_socket_queue(&tcp->u.client, buffer) != 0)
	{
		buffer_delete(buffer);
		return -1;
	}
	buffer_delete(buffer);
	return 0;
}


/* useful */
/* tcp_error */
static int _tcp_error(char const * message, int code)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %d)\n", __func__, message, code);
#endif
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
	_tcp_socket_init_fd(tcpsocket, tcp, tcpsocket->fd, NULL, 0);
	/* set the socket as non-blocking */
	if((flags = fcntl(tcpsocket->fd, F_GETFL)) == -1)
		return -_tcp_error("fcntl", 1);
	if((flags & O_NONBLOCK) == 0)
		if(fcntl(tcpsocket->fd, F_SETFL, flags | O_NONBLOCK) == -1)
			return -_tcp_error("fcntl", 1);
	return 0;
}


/* tcp_socket_init_fd */
static void _tcp_socket_init_fd(TCPSocket * tcpsocket, TCP * tcp, int fd,
		struct sockaddr * sa, socklen_t sa_size)
{
	tcpsocket->tcp = tcp;
	tcpsocket->client = NULL;
	tcpsocket->fd = fd;
	tcpsocket->sa = sa;
	tcpsocket->sa_size = sa_size;
	tcpsocket->bufin = NULL;
	tcpsocket->bufin_cnt = 0;
	tcpsocket->bufout = NULL;
	tcpsocket->bufout_cnt = 0;
}


/* tcp_socket_new_fd */
static TCPSocket * _tcp_socket_new_fd(TCP * tcp, int fd, struct sockaddr * sa,
		socklen_t sa_size)
{
	TCPSocket * tcpsocket;

	if((tcpsocket = object_new(sizeof(*tcpsocket))) == NULL)
		return NULL;
	_tcp_socket_init_fd(tcpsocket, tcp, fd, sa, sa_size);
	return tcpsocket;
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
		socklen_t sa_size);

static int _tcp_callback_accept(int fd, TCP * tcp)
{
	struct sockaddr * sa;
	socklen_t sa_size = tcp->ai_addrlen;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check parameters */
	if(tcp->u.server.fd != fd)
		return -1;
	if((sa = malloc(sa_size)) == NULL)
		/* XXX this may not be enough to recover */
		sa_size = 0;
	if((fd = accept(fd, sa, &sa_size)) < 0)
	{
		free(sa);
		return _tcp_error("accept", 1);
	}
	if(_accept_client(tcp, fd, sa, sa_size) != 0)
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
		socklen_t sa_size)
{
	TCPSocket * tcpsocket;
	Buffer * buf;
	char const banner[4] = "App!";

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	if((tcpsocket = _tcp_socket_new_fd(tcp, fd, sa, sa_size)) == NULL)
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
	size_t size;
	Variable * variable;
	Buffer * buffer;
	AppMessage * message = NULL;

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
	else
		tcpsocket->bufin_cnt += ssize;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() read() => %ld\n", __func__, ssize);
#endif
	size = tcpsocket->bufin_cnt;
	/* deserialize the data as a buffer (containing a message) */
	if((variable = variable_new_deserialize_type(VT_BUFFER, &size,
					tcpsocket->bufin)) == NULL)
		/* XXX assumes not enough data was available */
		return 0;
	tcpsocket->bufin_cnt -= size;
	memmove(tcpsocket->bufin, &tcpsocket->bufin[size],
			tcpsocket->bufin_cnt);
	if((variable_get_as(variable, VT_BUFFER, &buffer)) == 0)
	{
		message = appmessage_new_deserialize(buffer);
		buffer_delete(buffer);
	}
	if(message != NULL)
	{
		/* FIXME report the message */
		appmessage_delete(message);
	}
	else
	{
		/* FIXME report the error */
	}
	variable_delete(variable);
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
	/* XXX use a sliding cursor instead (and then queue the next message) */
	memmove(tcpsocket->bufout, &tcpsocket->bufout[ssize],
			tcpsocket->bufout_cnt - ssize);
	tcpsocket->bufout_cnt -= ssize;
	/* unregister the callback if there is nothing left to write */
	if(tcpsocket->bufout_cnt == 0)
		return 1;
	return 0;
}
