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



#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <System.h>
#include "App/appmessage.h"
#include "apptransport.h"
#include "../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR		PREFIX "/lib"
#endif


/* AppTransport */
/* private */
/* types */
struct _AppTransport
{
	AppTransportHelper helper;

	/* plug-in */
	AppTransportPluginHelper thelper;
	Plugin * plugin;
	AppTransportPlugin * tplugin;
	AppTransportPluginDefinition * definition;
};


/* prototypes */
/* helpers */
static int _apptransport_helper_status(AppTransport * transport,
		AppTransportStatus status, unsigned int code,
		char const * message);

static AppTransportClient * _apptransport_helper_client_new(
		AppTransport * transport);
static void _apptransport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client);

static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message);


/* protected */
/* functions */
/* apptransport_new */
static void _new_helper(AppTransport * transport, Event * event);

AppTransport * apptransport_new(AppTransportMode mode,
		AppTransportHelper * helper, char const * plugin,
		char const * name, Event * event)
{
	AppTransport * transport;

	/* allocate the transport */
	if((transport = object_new(sizeof(*transport))) == NULL)
		return NULL;
	memset(transport, 0, sizeof(*transport));
	transport->helper = *helper;
	/* initialize the plug-in helper */
	_new_helper(transport, event);
	/* load the transport plug-in */
	if((transport->plugin = plugin_new(LIBDIR, "App", "transport", plugin))
			== NULL
			|| (transport->definition = plugin_lookup(
					transport->plugin, "transport")) == NULL
			|| transport->definition->init == NULL
			|| transport->definition->destroy == NULL
			|| (transport->tplugin = transport->definition->init(
					&transport->thelper, mode, name))
			== NULL)
	{
		apptransport_delete(transport);
		return NULL;
	}
	return transport;
}

static void _new_helper(AppTransport * transport, Event * event)
{
	transport->thelper.transport = transport;
	transport->thelper.event = event;
	transport->thelper.status = _apptransport_helper_status;
	transport->thelper.client_new = _apptransport_helper_client_new;
	transport->thelper.client_delete = _apptransport_helper_client_delete;
	transport->thelper.client_receive = _apptransport_helper_client_receive;
}


/* apptransport_delete */
void apptransport_delete(AppTransport * transport)
{
	if(transport->tplugin != NULL)
		transport->definition->destroy(transport->tplugin);
	if(transport->plugin != NULL)
		plugin_delete(transport->plugin);
	object_delete(transport);
}


/* useful */
/* apptransport_send */
int apptransport_send(AppTransport * transport, AppMessage * message,
		int acknowledge)
{
	return transport->definition->send(transport->tplugin, message,
			acknowledge);
}


/* private */
/* functions */
/* helpers */
/* apptransport_helper_status */
static int _apptransport_helper_status(AppTransport * transport,
		AppTransportStatus status, unsigned int code,
		char const * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, %u, \"%s\")\n", __func__, status, code,
			message);
#endif
	/* FIXME really implement */
	return 0;
}


/* apptransport_helper_client_new */
static AppTransportClient * _apptransport_helper_client_new(
		AppTransport * transport)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* FIXME really implement */
	return NULL;
}


/* apptransport_helper_client_delete */
static void _apptransport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* FIXME really implement */
}


/* apptransport_helper_client_receive */
static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u \"%s\"\n", __func__,
			appmessage_get_type(message),
			appmessage_get_method(message));
#endif
	transport->helper.message(transport->helper.data, transport, client,
			message);
	return 0;
}
