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
	AppTransportPluginHelper helper;
	Plugin * plugin;
	AppTransportPlugin * tplugin;
	AppTransportPluginDefinition * definition;
};


/* prototypes */
/* helpers */
static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message);


/* protected */
/* functions */
/* apptransport_new */
static void _new_helper(AppTransport * transport);

AppTransport * apptransport_new(AppTransportMode mode, char const * plugin,
		char const * name)
{
	AppTransport * transport;

	/* allocate the transport */
	if((transport = object_new(sizeof(*transport))) == NULL)
		return NULL;
	memset(transport, 0, sizeof(*transport));
	/* initialize the helper */
	_new_helper(transport);
	/* load the transport plug-in */
	if((transport->plugin = plugin_new(LIBDIR, "App", "transport", plugin))
			== NULL
			|| (transport->definition = plugin_lookup(
					transport->plugin, "transport")) == NULL
			|| transport->definition->init == NULL
			|| transport->definition->destroy == NULL
			|| (transport->tplugin = transport->definition->init(
					&transport->helper, mode, name))
			== NULL)
	{
		apptransport_delete(transport);
		return NULL;
	}
	return transport;
}

static void _new_helper(AppTransport * transport)
{
	transport->helper.transport = transport;
	transport->helper.client_receive = _apptransport_helper_client_receive;
	/* FIXME really implement */
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


/* private */
/* functions */
/* helpers */
/* apptransport_helper_client_receive */
static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u \"%s\"\n", __func__,
			appmessage_get_type(message),
			appmessage_get_method(message));
#endif
	/* FIXME implement */
	return 0;
}
