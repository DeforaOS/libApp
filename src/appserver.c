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



#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <errno.h>
#include <System.h>
#include "apptransport.h"
#include "appinterface.h"
#include "App/appmessage.h"
#include "App/appserver.h"
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
static int _appserver_helper_message(AppServer * appserver,
		AppTransport * transport, AppTransportClient * client,
		AppMessage * message);


/* public */
/* functions */
/* appserver_new */
AppServer * appserver_new(const char * app)
{
	AppServer * appserver;
	Event * event;

	if((event = event_new()) == NULL)
		return NULL;
	if((appserver = appserver_new_event(app, event)) == NULL)
	{
		event_delete(event);
		return NULL;
	}
	appserver->event_free = 1;
	return appserver;
}


/* appserver_new_event */
static int _new_server(AppServer * appserver, char const * app);

AppServer * appserver_new_event(char const * app, Event * event)
{
	AppServer * appserver;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, app);
#endif
	if((appserver = object_new(sizeof(*appserver))) == NULL)
		return NULL;
	appserver->interface = NULL;
	appserver->event = event;
	appserver->event_free = 0;
	appserver->transport = NULL;
	if(_new_server(appserver, app) != 0)
	{
		appserver_delete(appserver);
		return NULL;
	}
	return appserver;
}

static int _new_server(AppServer * appserver, char const * app)
{
	if((appserver->interface = appinterface_new_server(app)) == NULL)
		return -1;
	appserver->helper.data = appserver;
	appserver->helper.message = _appserver_helper_message;
	/* FIXME hard-coded */
	if((appserver->transport = apptransport_new(ATM_SERVER,
					&appserver->helper, "tcp",
					"127.0.0.1:4250", appserver->event))
			== NULL)
		return -1;
	return 0;
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


/* appserver_get_client_id */
void * appserver_get_client_id(AppServer * appserver)
{
	/* FIXME really (re-)implement? */
	return NULL;
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
/* functions */
/* appserver_helper_message */
static int _appserver_helper_message(AppServer * appserver,
		AppTransport * transport, AppTransportClient * client,
		AppMessage * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__,
			appmessage_get_type(message));
#endif
	switch(appmessage_get_type(message))
	{
		case AMT_CALL:
			appinterface_call_process(appserver->interface,
					message);
			break;
		case AMT_CALL_RESPONSE:
			break;
	}
	return 0;
}
