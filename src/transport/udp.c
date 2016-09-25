/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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
/* TODO:
 * - expire clients after a timeout */



#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <time.h>
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

/* for udp4 and udp6 */
#ifndef UDP_DOMAIN
# define UDP_DOMAIN AF_UNSPEC
#endif


/* UDP */
/* private */
/* types */
typedef struct _AppTransportPlugin UDP;

typedef struct _UDPClient
{
	AppTransportClient * client;

	time_t time;
	struct sockaddr * sa;
	socklen_t sa_len;
} UDPClient;

typedef struct _UDPMessage
{
	char * buffer;
	size_t buffer_cnt;
} UDPMessage;

struct _AppTransportPlugin
{
	AppTransportPluginHelper * helper;
	AppTransportMode mode;
	int fd;

	struct addrinfo * ai;
	struct addrinfo * aip;

	union
	{
		struct
		{
			/* for servers */
			UDPClient ** clients;
			size_t clients_cnt;
		} server;
	} u;

	/* output queue */
	UDPMessage * messages;
	size_t messages_cnt;
};

#include "common.h"
#include "common.c"


/* protected */
/* prototypes */
/* plug-in */
static UDP * _udp_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name);
static void _udp_destroy(UDP * udp);

static int _udp_client_send(UDP * udp, AppTransportClient * client,
		AppMessage * message);
static int _udp_send(UDP * udp, AppMessage * message);

/* useful */
static int _udp_error(char const * message);

/* clients */
static int _udp_client_init(UDPClient * client, struct sockaddr * sa,
		socklen_t sa_len, UDP * udp);

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
	_udp_send,
	_udp_client_send
};


/* protected */
/* functions */
/* plug-in */
/* udp_init */
static int _init_client(UDP * udp, char const * name, int domain);
static int _init_server(UDP * udp, char const * name, int domain);
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
	udp->ai = NULL;
	udp->aip = NULL;
	udp->messages = NULL;
	udp->messages_cnt = 0;
	switch((udp->mode) = mode)
	{
		case ATM_CLIENT:
			res = _init_client(udp, name, UDP_DOMAIN);
			break;
		case ATM_SERVER:
			res = _init_server(udp, name, UDP_DOMAIN);
			break;
		default:
			res = -error_set_code(-EINVAL, "%s",
					"Unknown transport mode");
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

static int _init_client(UDP * udp, char const * name, int domain)
{
	memset(&udp->u, 0, sizeof(udp->u));
	/* obtain the remote address */
	if((udp->ai = _init_address(name, domain, 0)) == NULL)
		return -1;
	for(udp->aip = udp->ai; udp->aip != NULL; udp->aip = udp->aip->ai_next)
	{
		/* create the socket */
		if(_init_socket(udp) != 0)
			continue;
		/* listen for incoming messages */
		event_register_io_read(udp->helper->event, udp->fd,
				(EventIOFunc)_udp_callback_read, udp);
		break;
	}
	if(udp->aip == NULL)
	{
		freeaddrinfo(udp->ai);
		udp->ai = NULL;
		return -1;
	}
	return 0;
}

static int _init_server(UDP * udp, char const * name, int domain)
{
	udp->u.server.clients = NULL;
	udp->u.server.clients_cnt = 0;
	/* obtain the local address */
	if((udp->ai = _init_address(name, domain, AI_PASSIVE)) == NULL)
		return -1;
	for(udp->aip = udp->ai; udp->aip != NULL; udp->aip = udp->aip->ai_next)
	{
		/* create the socket */
		if(_init_socket(udp) != 0)
			continue;
		/* accept incoming messages */
		if(bind(udp->fd, udp->aip->ai_addr, udp->aip->ai_addrlen) != 0)
		{
			_udp_error("bind");
			close(udp->fd);
			udp->fd = -1;
			continue;
		}
		/* listen for incoming messages */
		event_register_io_read(udp->helper->event, udp->fd,
				(EventIOFunc)_udp_callback_read, udp);
		break;
	}
	if(udp->aip == NULL)
	{
		freeaddrinfo(udp->ai);
		udp->ai = NULL;
		return -1;
	}
	return 0;
}

static int _init_socket(UDP * udp)
{
	int flags;

	if((udp->fd = socket(udp->aip->ai_family, SOCK_DGRAM, 0)) < 0)
		return -_udp_error("socket");
	/* set the socket as non-blocking */
	if((flags = fcntl(udp->fd, F_GETFL)) == -1)
		return -_udp_error("fcntl");
	if((flags & O_NONBLOCK) == 0)
		if(fcntl(udp->fd, F_SETFL, flags | O_NONBLOCK) == -1)
			return -_udp_error("fcntl");
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() => %d\n", __func__, 0);
#endif
	return 0;
}


/* udp_destroy */
static void _destroy_server(UDP * udp);

static void _udp_destroy(UDP * udp)
{
	switch(udp->mode)
	{
		case ATM_CLIENT:
			break;
		case ATM_SERVER:
			_destroy_server(udp);
			break;
	}
	if(udp->fd >= 0)
		close(udp->fd);
	free(udp->messages);
	if(udp->ai != NULL)
		freeaddrinfo(udp->ai);
	object_delete(udp);
}

static void _destroy_server(UDP * udp)
{
	AppTransportPluginHelper * helper = udp->helper;
	size_t i;

	for(i = 0; i < udp->u.server.clients_cnt; i++)
	{
		helper->client_delete(helper->transport,
				udp->u.server.clients[i]->client);
		free(udp->u.server.clients[i]->sa);
		free(udp->u.server.clients[i]);
	}
}


/* udp_client_send */
static int _udp_client_send(UDP * udp, AppTransportClient * client,
		AppMessage * message)
{
	int ret;
	size_t i;
	UDPClient * c;
	Buffer * buffer;
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	/* lookup the client */
	for(i = 0; i < udp->u.server.clients_cnt; i++)
	{
		c = udp->u.server.clients[i];
		if(c->client == client)
			break;
	}
	if(i == udp->u.server.clients_cnt)
		return -error_set_code(-ENOENT, "%s", "Unknown client");
	/* send the message */
	if((buffer = buffer_new(0, NULL)) == NULL)
		return -1;
	if(appmessage_serialize(message, buffer) != 0)
	{
		buffer_delete(buffer);
		return -1;
	}
#ifdef DEBUG
	if(udp->aip->ai_family == AF_INET)
	{
		sa = (struct sockaddr_in *)udp->aip->ai_addr;
		fprintf(stderr, "DEBUG: %s() %s (%s:%u) size=%lu\n", __func__,
				"sendto()", inet_ntoa(sa->sin_addr),
				ntohs(sa->sin_port), buffer_get_size(buffer));
	}
	else
		fprintf(stderr, "DEBUG: %s() %s domain=%d size=%lu\n", __func__,
				"sendto()", udp->aip->ai_family,
				buffer_get_size(buffer));
#endif
	ret = sendto(udp->fd, buffer_get_data(buffer), buffer_get_size(buffer),
			0, c->sa, c->sa_len);
	buffer_delete(buffer);
	return ret;
}


/* udp_send */
static int _udp_send(UDP * udp, AppMessage * message)
{
	int ret;
	Buffer * buffer;
#ifdef DEBUG
	struct sockaddr_in * sa;
#endif

	if(udp->mode != ATM_CLIENT)
		return -error_set_code(-EINVAL, "%s", "Not a client");
	/* send the message */
	if((buffer = buffer_new(0, NULL)) == NULL)
		return -1;
	if(appmessage_serialize(message, buffer) != 0)
	{
		buffer_delete(buffer);
		return -1;
	}
#ifdef DEBUG
	if(udp->aip->ai_family == AF_INET)
	{
		sa = (struct sockaddr_in *)udp->aip->ai_addr;
		fprintf(stderr, "DEBUG: %s() %s (%s:%u) size=%lu\n", __func__,
				"sendto()", inet_ntoa(sa->sin_addr),
				ntohs(sa->sin_port), buffer_get_size(buffer));
	}
	else
		fprintf(stderr, "DEBUG: %s() %s domain=%d size=%lu\n", __func__,
				"sendto()", udp->aip->ai_family,
				buffer_get_size(buffer));
#endif
	ret = sendto(udp->fd, buffer_get_data(buffer), buffer_get_size(buffer),
			0, udp->aip->ai_addr, udp->aip->ai_addrlen);
	buffer_delete(buffer);
	return ret;
}


/* useful */
/* udp_error */
static int _udp_error(char const * message)
{
	return error_set_code(-errno, "%s%s%s",
			(message != NULL) ? message : "",
			(message != NULL) ? ": " : "", strerror(errno));
}


/* clients */
/* udp_client_init */
static int _udp_client_init(UDPClient * client, struct sockaddr * sa,
		socklen_t sa_len, UDP * udp)
{
	AppTransportPluginHelper * helper = udp->helper;
#ifndef NI_MAXHOST
# define NI_MAXHOST 256
#endif
	char host[NI_MAXHOST];
	char const * name = host;
	const int flags = NI_NUMERICSERV | NI_DGRAM;

	if((client->sa = malloc(sa_len)) == NULL)
		return -1;
	/* XXX may not be instant */
	if(getnameinfo(client->sa, client->sa_len, host, sizeof(host), NULL, 0,
				NI_NAMEREQD | flags) != 0
			&& getnameinfo(client->sa, client->sa_len, host,
				sizeof(host), NULL, 0, NI_NUMERICHOST | flags)
			!= 0)
		name = NULL;
	if((client->client = helper->client_new(helper->transport, name))
			== NULL)
	{
		free(client->sa);
		return -1;
	}
	/* XXX we can ignore errors here */
	client->time = time(NULL);
	memcpy(client->sa, sa, sa_len);
	client->sa_len = sa_len;
	return 0;
}


/* callbacks */
/* udp_callback_read */
static void _callback_read_client(UDP * udp, struct sockaddr * sa,
		socklen_t sa_len, AppMessage * message);
static void _callback_read_server(UDP * udp, struct sockaddr * sa,
		socklen_t sa_len, AppMessage * message);

static int _udp_callback_read(int fd, UDP * udp)
{
	char buf[65536];
	ssize_t ssize;
	struct sockaddr * sa;
	socklen_t sa_len = udp->aip->ai_addrlen;
	size_t size;
	Buffer * buffer;
	AppMessage * message = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d)\n", __func__, fd);
#endif
	/* check arguments */
	if(fd != udp->fd)
		return -1;
	if((sa = malloc(sa_len)) == NULL)
		return -_udp_error(NULL);
	if((ssize = recvfrom(fd, buf, sizeof(buf), 0, sa, &sa_len)) < 0)
	{
		/* XXX report error (and re-open the socket) */
		_udp_error("recvfrom");
		free(sa);
		close(udp->fd);
		udp->fd = -1;
		return -1;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() ssize=%ld\n", __func__, ssize);
#endif
	size = ssize;
	if((buffer = buffer_new(ssize, buf)) == NULL)
	{
		free(sa);
		return 0;
	}
	message = appmessage_new_deserialize(buffer);
	buffer_delete(buffer);
	if(message == NULL)
	{
		/* FIXME report error */
		free(sa);
		return 0;
	}
	switch(udp->mode)
	{
		case ATM_CLIENT:
			_callback_read_client(udp, sa, sa_len, message);
			break;
		case ATM_SERVER:
			_callback_read_server(udp, sa, sa_len, message);
			break;
	}
	appmessage_delete(message);
	free(sa);
	return 0;
}

static void _callback_read_client(UDP * udp, struct sockaddr * sa,
		socklen_t sa_len, AppMessage * message)
{
	AppTransportPluginHelper * helper = udp->helper;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__,
			appmessage_get_type(message));
#endif
	if(sa_len != udp->aip->ai_addrlen
			|| memcmp(udp->aip->ai_addr, sa, sa_len) != 0)
		/* the message is not for us */
		return;
	helper->receive(helper->transport, message);
}

static void _callback_read_server(UDP * udp, struct sockaddr * sa,
		socklen_t sa_len, AppMessage * message)
{
	AppTransportPluginHelper * helper = udp->helper;
	size_t i;
	UDPClient ** p;
	AppTransportClient * client;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	for(i = 0; i < udp->u.server.clients_cnt; i++)
		if(udp->u.server.clients[i]->sa_len == sa_len
				&& memcmp(udp->u.server.clients[i]->sa, sa,
					sa_len) == 0)
			break;
	if(i == udp->u.server.clients_cnt)
	{
		if((p = realloc(udp->u.server.clients, sizeof(*p) * (i + 1)))
				== NULL)
			/* FIXME report error */
			return;
		udp->u.server.clients = p;
		if((udp->u.server.clients[i] = malloc(sizeof(**p))) == NULL)
			/* FIXME report error */
			return;
		if(_udp_client_init(udp->u.server.clients[i], sa, sa_len, udp)
				!= 0)
		{
			/* FIXME report error */
			free(p[i]);
			return;
		}
		udp->u.server.clients_cnt++;
	}
	else
		udp->u.server.clients[i]->time = time(NULL);
	client = udp->u.server.clients[i]->client;
	helper->client_receive(helper->transport, client, message);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() received message\n", __func__);
#endif
}
