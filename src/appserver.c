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
AppServer * appserver_new(AppServerOptions options, const char * app,
		char const * name)
{
	return appserver_new_event(options, app, name, NULL);
}


/* appserver_new_event */
AppServer * appserver_new_event(AppServerOptions options, char const * app,
		char const * name, Event * event)
{
	AppServer * appserver;

	if((appserver = object_new(sizeof(*appserver))) == NULL)
		return NULL;
	appserver->interface = appinterface_new_server(app);
	appserver->helper.data = appserver;
	appserver->helper.message = _appserver_helper_message;
	appserver->transport = apptransport_new_app(ATM_SERVER,
			&appserver->helper, app, name, event);
	appserver->event = (event != NULL) ? event : event_new();
	appserver->event_free = (event != NULL) ? 0 : 1;
	/* check for errors */
	if(appserver->interface == NULL || appserver->transport == NULL
			|| appserver->event == NULL)
	{
		appserver_delete(appserver);
		return NULL;
	}
	/* register the server if requested */
	if(options & ASO_REGISTER)
		/* XXX should we really ignore errors? */
		apptransport_server_register(appserver->transport, app);
	return appserver;
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


/* accessors */
/* appserver_get_app */
char const * appserver_get_app(AppServer * appserver)
{
	return appinterface_get_app(appserver->interface);
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
static int _helper_message_call(AppServer * appserver, AppTransport * transport,
		AppTransportClient * client, AppMessage * message);

static int _appserver_helper_message(void * data, AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	AppServer * appserver = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, %u)\n", __func__, (void *)client,
			appmessage_get_type(message));
#endif
	/* FIXME can it happen? */
	if(message == NULL)
		/* XXX report errors */
		return -1;
	switch(appmessage_get_type(message))
	{
		case AMT_CALL:
			return _helper_message_call(appserver, transport,
					client, message);
	}
	return -1;
}

static int _helper_message_call(AppServer * appserver, AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	int ret;
	String const * name;
	String const * method;
	Variable * result = NULL;

	name = (client != NULL) ? apptransport_client_get_name(client) : NULL;
	method = appmessage_get_method(message);
	if(!appinterface_can_call(appserver->interface, name, method))
		/* XXX report errors */
		return -1;
	ret = appinterface_call_variablev(appserver->interface, result, method,
			0, NULL);
	if(result != NULL)
		variable_delete(result);
	return ret;
}
