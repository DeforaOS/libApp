/* $Id$ */
/* Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App.h"

#ifndef PROGNAME
# define PROGNAME	"transport"
#endif


/* private */
/* types */
typedef struct _AppTransport
{
	int ret;
	AppTransportPluginHelper helper;
	AppTransportPluginDefinition * plugind;
	AppTransportPlugin * server;
	AppTransportPlugin * client;
	AppMessage * message;
} Transport;


/* prototypes */
static int _transport(char const * protocol, char const * name);

/* helpers */
static int _transport_helper_receive(AppTransport * transport,
		AppMessage * message);
static int _transport_helper_status(AppTransport * transport,
		AppTransportStatus status, unsigned int code,
		char const * message);

static AppTransportClient * _transport_helper_client_new(
		AppTransport * transport, char const * name);
static void _transport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client);
static int _transport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message);

/* callbacks */
static int _transport_callback_idle(void * data);
static int _transport_callback_timeout(void * data);

static int _usage(void);


/* functions */
/* transport */
static int _transport(char const * protocol, char const * name)
{
	char * cwd;
	Plugin * plugin;
	AppTransport transport;
	AppTransportPluginHelper * helper = &transport.helper;
	struct timeval tv;

	/* load the transport plug-in */
	if((cwd = getcwd(NULL, 0)) == NULL)
		return error_set_print(PROGNAME, 2, "%s", strerror(errno));
	/* XXX rather ugly but does the trick */
	plugin = plugin_new(cwd, "../src", "transport", protocol);
	free(cwd);
	if(plugin == NULL)
		return error_print(PROGNAME);
	transport.ret = 0;
	if((transport.plugind = plugin_lookup(plugin, "transport")) == NULL)
	{
		plugin_delete(plugin);
		return error_print(PROGNAME);
	}
	/* initialize the helper */
	memset(helper, 0, sizeof(*helper));
	helper->transport = &transport;
	helper->event = event_new();
	helper->receive = _transport_helper_receive;
	helper->status = _transport_helper_status;
	helper->client_new = _transport_helper_client_new;
	helper->client_delete = _transport_helper_client_delete;
	helper->client_receive = _transport_helper_client_receive;
	/* create a server and a client */
	transport.server = (helper->event != NULL)
		? transport.plugind->init(helper, ATM_SERVER, name) : NULL;
	transport.client = (helper->event != NULL)
		? transport.plugind->init(helper, ATM_CLIENT, name) : NULL;
	if(helper->event == NULL || transport.server == NULL
			|| transport.client == NULL)
	{
		if(helper->event != NULL)
			event_delete(helper->event);
		if(transport.client != NULL)
			transport.plugind->destroy(transport.client);
		if(transport.server != NULL)
			transport.plugind->destroy(transport.server);
		plugin_delete(plugin);
		return error_print(PROGNAME);
	}
	transport.message = appmessage_new_callv("hello", -1);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/* enter the main loop */
	if(event_register_idle(helper->event, _transport_callback_idle,
				&transport) != 0
			|| event_register_timeout(helper->event, &tv,
				_transport_callback_timeout, &transport) != 0
			|| event_loop(helper->event) != 0)
	{
		error_print(PROGNAME);
		transport.ret = -1;
	}
	else if(transport.ret != 0)
		error_print(PROGNAME);
	appmessage_delete(transport.message);
	transport.plugind->destroy(transport.client);
	transport.plugind->destroy(transport.server);
	event_delete(helper->event);
	plugin_delete(plugin);
	return transport.ret;
}


/* helpers */
/* transport_helper_client_new */
static AppTransportClient * _transport_helper_client_new(
		AppTransport * transport, char const * name)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, name);
#endif
	/* FIXME really implement */
	return (AppTransportClient *)transport;
}


/* transport_helper_client_delete */
static void _transport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client)
{
}


/* transport_helper_client_receive */
static int _transport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	String const * method;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u \"%s\"\n", __func__,
			appmessage_get_type(message),
			appmessage_get_method(message));
#endif
	if(appmessage_get_type(message) == AMT_CALL
			&& (method = appmessage_get_method(message)) != NULL
			&& strcmp(method, "hello") == 0)
		event_loop_quit(transport->helper.event);
	return 0;
}


/* transport_helper_receive */
static int _transport_helper_receive(AppTransport * transport,
		AppMessage * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	return 0;
}


/* transport_helper_status */
static int _transport_helper_status(AppTransport * transport,
		AppTransportStatus status, unsigned int code,
		char const * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, %u, \"%s\")\n", __func__, status, code,
			message);
#endif
	return 0;
}


/* callbacks */
/* transport_callback_idle */
static int _transport_callback_idle(void * data)
{
	Transport * transport = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	transport->plugind->client_send(transport->client, transport->message);
	return 1;
}


/* transport_callback_timeout */
static int _transport_callback_timeout(void * data)
{
	Transport * transport = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	event_loop_quit(transport->helper.event);
	/* report the error */
	transport->ret = error_set_code(2, "%s", "Timeout");
	return 1;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: transport [-p protocol] [name]\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	char const * protocol = "udp";
	char const * name = "127.0.0.1:4242";
	int o;

	while((o = getopt(argc, argv, "p:")) != -1)
		switch(o)
		{
			case 'p':
				protocol = optarg;
				break;
			default:
				return _usage();
		}
	if(optind == argc - 1)
		name = argv[optind];
	else if(optind != argc)
		return _usage();
	return (_transport(protocol, name) == 0) ? 0 : 2;
}
