/* $Id$ */
/* Copyright (c) 2011-2014 Pierre Pronchery <khorben@defora.org> */
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



#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App/appmessage.h"
#include "App/appserver.h"
#include "apptransport.h"
#include "appinterface.h"
#include "../config.h"


/* AppServer */
/* private */
/* types */
struct _AppServer
{
	AppInterface * interface;
	Event * event;
	int event_free;
	AppTransport * transport;
	AppTransportHelper helper;
};


/* prototypes */
/* helpers */
static int _appserver_helper_message(void * data, AppTransport * transport,
		AppTransportClient * client, AppMessage * message);


/* public */
/* functions */
/* appserver_new */
AppServer * appserver_new(const char * app, char const * name)
{
	AppServer * appserver;
	Event * event;

	if((event = event_new()) == NULL)
		return NULL;
	if((appserver = appserver_new_event(app, name, event)) == NULL)
	{
		event_delete(event);
		return NULL;
	}
	appserver->event_free = 1;
	return appserver;
}


/* appserver_new_event */
static AppTransport * _new_event_transport(AppTransportHelper * helper,
		Event * event, char const * app, char const * name);
static String * _new_server_name(char const * app, char const * name);
static String * _new_server_transport(String ** name);

AppServer * appserver_new_event(char const * app, char const * name,
		Event * event)
{
	AppServer * appserver;

	if((appserver = object_new(sizeof(*appserver))) == NULL)
		return NULL;
	appserver->interface = appinterface_new_server(app);
	appserver->helper.data = appserver;
	appserver->helper.message = _appserver_helper_message;
	appserver->transport = _new_event_transport(&appserver->helper, event,
			app, name);
	appserver->event = event;
	appserver->event_free = 0;
	/* check for errors */
	if(appserver->interface == NULL || appserver->transport == NULL)
	{
		appserver_delete(appserver);
		return NULL;
	}
	return appserver;
}

static AppTransport * _new_event_transport(AppTransportHelper * helper,
		Event * event, char const * app, char const * name)
{
	AppTransport * ret;
	String * n;
	String * transport;

	if((n = _new_server_name(app, name)) == NULL)
		return NULL;
	if((transport = _new_server_transport(&n)) == NULL)
	{
		string_delete(n);
		return NULL;
	}
	ret = apptransport_new(ATM_SERVER, helper, transport, n, event);
	string_delete(transport);
	string_delete(n);
	return ret;
}

static String * _new_server_name(char const * app, char const * name)
{
	String * var;

	if(name != NULL)
		return string_new(name);
	/* obtain the desired transport and name from the environment */
	if((var = string_new_append("APPSERVER_", app, NULL)) == NULL)
		return NULL;
	name = getenv(var);
	string_delete(var);
	return string_new(name);
}

static String * _new_server_transport(String ** name)
{
	String * p;
	String * transport;

	if((p = strchr(*name, ':')) == NULL)
		/* XXX hard-coded default value */
		return string_new("tcp");
	/* XXX */
	*(p++) = '\0';
	transport = *name;
	if((*name = string_new(p)) == NULL)
	{
		string_delete(transport);
		return NULL;
	}
	return transport;
}


/* appserver_delete */
void appserver_delete(AppServer * appserver)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(appserver->interface != NULL)
		appinterface_delete(appserver->interface);
	if(appserver->event_free != 0)
		event_delete(appserver->event);
	object_delete(appserver);
}


/* useful */
/* appserver_loop */
int appserver_loop(AppServer * appserver)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	return event_loop(appserver->event);
}


/* private */
/* appserver_helper_message */
static int _appserver_helper_message(void * data, AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	AppServer * appserver = data;

	/* FIXME implement */
	return 0;
}
