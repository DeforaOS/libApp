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



#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <System.h>
#include "App/appclient.h"
#include "appmessage.h"
#include "apptransport.h"
#include "../config.h"
/* FIXME:
 * clarify parsing name strings (if there is a colon, there is a transport,
 * although possibly empty) */

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
	AppTransportMode mode;
	AppTransportHelper helper;
	String * name;

	/* registration */
	AppClient * appclient;

	/* plug-in */
	AppTransportPluginHelper thelper;
	Plugin * plugin;
	AppTransportPlugin * tplugin;
	AppTransportPluginDefinition * definition;

	/* acknowledgements */
	AppMessageID id;
};

struct _AppTransportClient
{
	AppTransport * transport;
	String * name;
};


/* prototypes */
/* helpers */
static int _apptransport_helper_status(AppTransport * transport,
		AppTransportStatus status, unsigned int code,
		char const * message);

static AppTransportClient * _apptransport_helper_client_new(
		AppTransport * transport, char const * name);
static void _apptransport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client);

static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message);


/* protected */
/* functions */
/* apptransport_new */
static void _new_helper(AppTransport * transport, AppTransportMode mode,
		Event * event);

AppTransport * apptransport_new(AppTransportMode mode,
		AppTransportHelper * helper, char const * plugin,
		char const * name, Event * event)
{
	AppTransport * transport;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, \"%s\", \"%s\", %p)\n", __func__, mode,
			plugin, name, (void *)event);
#endif
	/* check the arguments */
	if(plugin == NULL || plugin[0] == '\0')
	{
		error_set_code(1, "%s", "Invalid transport");
		return NULL;
	}
	if(name == NULL || event == NULL)
	{
		error_set_code(1, "%s", "Invalid arguments");
		return NULL;
	}
	/* allocate the transport */
	if((transport = object_new(sizeof(*transport))) == NULL)
		return NULL;
	memset(transport, 0, sizeof(*transport));
	transport->mode = mode;
	if(helper != NULL)
		transport->helper = *helper;
	transport->name = string_new(name);
	/* initialize the plug-in helper */
	_new_helper(transport, mode, event);
	/* load the transport plug-in */
	if(transport->name == NULL
			|| (transport->plugin = plugin_new(LIBDIR, "App",
					"transport", plugin)) == NULL
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

static void _new_helper(AppTransport * transport, AppTransportMode mode,
		Event * event)
{
	transport->thelper.transport = transport;
	transport->thelper.event = event;
	transport->thelper.status = _apptransport_helper_status;
	transport->thelper.client_new = _apptransport_helper_client_new;
	transport->thelper.client_delete = _apptransport_helper_client_delete;
	transport->thelper.client_receive = _apptransport_helper_client_receive;
}


/* apptransport_new_app */
static String * _new_app_name(char const * app, char const * name);
static String * _new_app_transport(String ** name);

AppTransport * apptransport_new_app(AppTransportMode mode,
		AppTransportHelper * helper, char const * app,
		char const * name, Event * event)
{
	AppTransport * apptransport;
	String * n;
	String * transport;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, \"%s\", \"%s\")\n", __func__, mode, app,
			name);
#endif
	if((n = _new_app_name(app, name)) == NULL)
		return NULL;
	if((transport = _new_app_transport(&n)) == NULL)
	{
		string_delete(n);
		return NULL;
	}
	apptransport = apptransport_new(mode, helper, transport, n, event);
	string_delete(transport);
	string_delete(n);
	return apptransport;
}

static String * _new_app_name(char const * app, char const * name)
{
	String * var;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", \"%s\")\n", __func__, app, name);
#endif
	if(app == NULL || app[0] == '\0')
	{
		error_set_code(1, "%s", "Invalid App");
		return NULL;
	}
	if(name != NULL)
		return string_new(name);
	/* obtain the desired transport and name from the environment */
	if((var = string_new_append("APPSERVER_", app, NULL)) == NULL)
		return NULL;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, var);
#endif
	name = getenv(var);
	string_delete(var);
	if(name == NULL)
		return apptransport_lookup(app);
	return string_new(name);
}

static String * _new_app_transport(String ** name)
{
	String * p;
	String * transport;

	if((p = strchr(*name, ':')) == NULL)
		/* XXX hard-coded default value */
		return string_new("tcp");
	/* XXX */
	*p = '\0';
	transport = string_new(*name);
	*p = ':';
	if(transport == NULL || (*name = string_new(++p)) == NULL)
	{
		string_delete(transport);
		return NULL;
	}
	return transport;
}


/* apptransport_delete */
void apptransport_delete(AppTransport * transport)
{
	if(transport->appclient != NULL)
		appclient_delete(transport->appclient);
	if(transport->tplugin != NULL)
		transport->definition->destroy(transport->tplugin);
	if(transport->plugin != NULL)
		plugin_delete(transport->plugin);
	if(transport->name != NULL)
		string_delete(transport->name);
	object_delete(transport);
}


/* accessors */
/* apptransport_get_mode */
AppTransportMode apptransport_get_mode(AppTransport * transport)
{
	return transport->mode;
}


/* apptransport_get_name */
String const * apptransport_get_name(AppTransport * transport)
{
	return transport->name;
}


/* apptransport_get_transport */
String const * apptransport_get_transport(AppTransport * transport)
{
	return transport->definition->name;
}


/* apptransport_client_get_name */
String const * apptransport_client_get_name(AppTransportClient * client)
{
	return client->name;
}


/* useful */
/* apptransport_lookup */
String * apptransport_lookup(char const * app)
{
	const char session[] = "Session";
	String * name = NULL;
	AppClient * appclient;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, app);
#endif
	if(strcmp(app, session) == 0)
	{
		error_set_code(1, "%s: %s", app, "Could not lookup");
		return NULL;
	}
	if((appclient = appclient_new(NULL, session, NULL)) == NULL)
		return NULL;
	appclient_call(appclient, (void **)&name, "lookup", app);
	appclient_delete(appclient);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() => \"%s\"\n", __func__, name);
#endif
	if(name == NULL)
		error_set_code(1, "%s: %s", app, "Could not lookup");
	return name;
}


/* apptransport_client_send */
int apptransport_client_send(AppTransport * transport, AppMessage * message,
		int acknowledge)
{
	if(transport->mode == ATM_CLIENT
			&& appmessage_get_type(message) == AMT_CALL
			&& acknowledge != 0)
		/* FIXME will wrap around after 2^32-1 acknowledgements */
		appmessage_set_id(message, ++transport->id);
	return transport->definition->client_send(transport->tplugin, message);
}


/* apptransport_server_register */
int apptransport_server_register(AppTransport * transport, char const * app)
{
	int ret;
	int res = -1;
	const char session[] = "Session";

	if(transport->mode != ATM_SERVER)
		return -error_set_code(1, "%s",
				"Only servers can register to sessions");
	if(transport->appclient != NULL)
		appclient_delete(transport->appclient);
	if((transport->appclient = appclient_new(NULL, session, NULL)) == NULL)
		return -1;
	ret = appclient_call(transport->appclient, (void **)&res, "register",
			app, transport->name);
	ret = (ret == 0 && res == 0) ? 0 : -1;
	return ret;
}


/* apptransport_server_send */
int apptransport_server_send(AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	if(transport->mode != ATM_SERVER)
		return -error_set_code(1, "%s",
				"Only servers can reply to clients");
	if(transport->definition->server_send == NULL)
		return -error_set_code(1, "%s",
				"This transport does not support replies");
	return transport->definition->server_send(transport->tplugin, client,
			message);
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
		AppTransport * transport, char const * name)
{
	AppTransportClient * client;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((client = object_new(sizeof(*client))) == NULL)
		return NULL;
	client->transport = transport;
	client->name = (name != NULL) ? string_new(name) : NULL;
	return client;
}


/* apptransport_helper_client_delete */
static void _apptransport_helper_client_delete(AppTransport * transport,
		AppTransportClient * client)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	object_delete(client);
}


/* apptransport_helper_client_receive */
static int _apptransport_helper_client_receive(AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	AppMessageID id;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u %u \"%s\"\n", __func__,
			appmessage_get_type(message),
			appmessage_get_id(message),
			appmessage_get_method(message));
#endif
	if(transport->mode != ATM_SERVER)
		/* XXX improve the error message */
		return -error_set_code(1, "Not a server");
	/* XXX check for errors? */
	transport->helper.message(transport->helper.data, transport, client,
			message);
	/* check if an acknowledgement is requested */
	if((id = appmessage_get_id(message)) != 0)
		/* XXX we can ignore errors */
		if((message = appmessage_new_acknowledgement(id)) != NULL)
		{
			apptransport_server_send(transport, client, message);
			appmessage_delete(message);
		}
	return 0;
}
