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


/* UDP */
/* private */
/* types */
typedef struct _AppTransportPlugin UDP;

typedef struct _UDPMessage
{
	char * buffer;
	size_t buffer_cnt;
} UDPMessage;

struct _AppTransportPlugin
{
	AppTransportPluginHelper * helper;
	int fd;

	/* output queue */
	UDPMessage * messages;
	size_t messages_cnt;
};


/* protected */
/* prototypes */
/* plug-in */
static UDP * _udp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name);
static void _udp_destroy(UDP * udp);

static int _udp_send(UDP * udp, AppMessage * message, int acknowledge);

/* useful */
static int _udp_error(char const * message, int code);

/* callbacks */
static int _udp_callback_read(int fd, UDP * udp);


/* public */
/* constants */
/* plug-in */
AppTransportPluginDefinition transport =
{
	"UDP",
	NULL,
	_udp_init,
	_udp_destroy,
	_udp_send
};


/* protected */
/* functions */
/* plug-in */
/* udp_init */
static int _init_address(char const * name, struct sockaddr_in * sa);
static int _init_client(UDP * udp, char const * name);
static int _init_server(UDP * udp, char const * name);
static int _init_socket(UDP * udp);

static UDP * _udp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name)
{
	UDP * udp;
	int res;

	if((udp = object_new(sizeof(*udp))) == NULL)
		return NULL;
	udp->helper = helper;
	udp->fd = -1;
	udp->messages = NULL;
	udp->messages_cnt = 0;
	switch(mode)
	{
		case ATM_CLIENT:
			res = _init_client(udp, name);
			break;
		case ATM_SERVER:
			res = _init_server(udp, name);
			break;
		default:
			res = -error_set_code(1, "Unknown transport mode");
			break;
	}
	/* check for errors */
	if(res != 0)
	{
		_udp_destroy(udp);
		return NULL;
	}
	return udp;
}

static int _init_address(char const * name, struct sockaddr_in * sa)
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
	sa->sin_family = AF_INET;
	sa->sin_port = htons(l);
	memcpy(&sa->sin_addr, he->h_addr_list[0], sizeof(sa->sin_addr));
	return 0;
}

static int _init_client(UDP * udp, char const * name)
{
	struct sockaddr_in sa;

	/* obtain the remote address */
	if(_init_address(name, &sa) != 0)
		return -1;
	/* create the socket */
	if(_init_socket(udp) != 0)
		return -1;
	/* listen for incoming messages */
	event_register_io_read(udp->helper->event, udp->fd,
			(EventIOFunc)_udp_callback_read, udp);
	return 0;
}

static int _init_server(UDP * udp, char const * name)
{
	struct sockaddr_in sa;

	/* obtain the local address */
	if(_init_address(name, &sa) != 0)
		return -1;
	/* create the socket */
	if(_init_socket(udp) != 0)
		return -1;
	/* listen for incoming messages */
	event_register_io_read(udp->helper->event, udp->fd,
			(EventIOFunc)_udp_callback_read, udp);
	return 0;
}

static int _init_socket(UDP * udp)
{
	int flags;

	if((udp->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -_udp_error("socket", 1);
	/* set the socket as non-blocking */
	if((flags = fcntl(udp->fd, F_GETFL)) == -1)
		return -_udp_error("fcntl", 1);
	if((flags & O_NONBLOCK) == 0)
		if(fcntl(udp->fd, F_SETFL, flags | O_NONBLOCK) == -1)
			return -_udp_error("fcntl", 1);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() => %d\n", __func__, 0);
#endif
	return 0;
}


/* udp_destroy */
static void _udp_destroy(UDP * udp)
{
	if(udp->fd >= 0)
		close(udp->fd);
	free(udp->messages);
	object_delete(udp);
}


/* udp_send */
static int _udp_send(UDP * udp, AppMessage * message, int acknowledge)
{
	/* FIXME really implement */
	return -1;
}


/* useful */
/* udp_error */
static int _udp_error(char const * message, int code)
{
	return error_set_code(code, "%s%s%s", (message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
}


/* callbacks */
/* udp_callback_read */
static int _udp_callback_read(int fd, UDP * udp)
{
	char buf[65536];
	ssize_t ssize;
	struct sockaddr_in sa;
	socklen_t sa_len;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check arguments */
	if(fd != udp->fd)
		return -1;
	if((ssize = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&sa,
					&sa_len)) < 0)
	{
		/* XXX report error (and re-open the socket) */
		error_set_code(1, "%s: %s", "recvfrom", strerror(errno));
		close(udp->fd);
		udp->fd = -1;
		return -1;
	}
	else if(ssize == 0)
	{
		/* FIXME really implement */
	}
	else
	{
		/* FIXME really implement */
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() received message (%ld)\n", __func__,
			ssize);
#endif
	return 0;
}
