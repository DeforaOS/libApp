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
 * - consider the transport successful once connected */



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App.h"


/* private */
/* types */
typedef struct _Transport
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
static AppTransportClient * _transport_helper_client_new(
		AppTransport * transport);

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
	Transport transport;
	AppTransportPluginHelper * helper = &transport.helper;
	struct timeval tv;

	/* load the transport plug-in */
	if((cwd = getcwd(NULL, 0)) == NULL)
		return error_set_print("transport", 2, "%s", strerror(errno));
	/* XXX rather ugly but does the trick */
	plugin = plugin_new(cwd, "../src", "transport", protocol);
	free(cwd);
	if(plugin == NULL)
		return error_print("transport");
	transport.ret = 0;
	if((transport.plugind = plugin_lookup(plugin, "transport")) == NULL)
	{
		plugin_delete(plugin);
		return error_print("transport");
	}
	/* initialize the helper */
	memset(helper, 0, sizeof(*helper));
	helper->transport = &transport;
	helper->event = event_new();
	helper->client_new = _transport_helper_client_new;
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
		return error_print("transport");
	}
	transport.message = appmessage_new_call("hello");
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/* enter the main loop */
	if(event_register_idle(helper->event, _transport_callback_idle,
				&transport) != 0
			|| event_register_timeout(helper->event, &tv,
				_transport_callback_timeout, &transport) != 0
			|| event_loop(helper->event) != 0)
	{
		error_print("transport");
		transport.ret = -1;
	}
	else if(transport.ret != 0)
		error_print("transport");
	event_delete(helper->event);
	transport.plugind->destroy(transport.client);
	transport.plugind->destroy(transport.server);
	plugin_delete(plugin);
	return transport.ret;
}


/* helpers */
/* transport_helper_client_new */
static AppTransportClient * _transport_helper_client_new(
		AppTransport * transport)
{
	/* FIXME really implement */
	return (AppTransportClient*)transport;
}


/* callbacks */
/* transport_callback_idle */
static int _transport_callback_idle(void * data)
{
	Transport * transport = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	transport->plugind->send(transport->client, transport->message, 0);
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
	fputs("Usage: transport -p protocol name\n", stderr);
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
