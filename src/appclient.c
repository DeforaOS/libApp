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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "apptransport.h"
#include "appinterface.h"
#include "App/appmessage.h"
#include "App/appclient.h"


/* AppClient */
/* private */
/* types */
struct _AppClient
{
	AppInterface * interface;
	Event * event;
	AppTransport * transport;
	AppTransportHelper helper;
};


/* prototypes */
/* helper */
static int _appclient_helper_message(AppClient * appclient,
		AppTransport * transport, AppTransportClient * client,
		AppMessage * message);


/* public */
/* functions */
/* appclient_new */
AppClient * appclient_new(char const * app)
{
	AppClient * appclient;
	Event * event;

	if((event = event_new()) == NULL)
		return NULL;
	if((appclient = appclient_new_event(app, event)) == NULL)
	{
		event_delete(event);
		return NULL;
	}
	return appclient;
}


/* appclient_new_event */
AppClient * appclient_new_event(char const * app, Event * event)
{
	AppClient * appclient;

#ifdef DEBUG
	fprintf(stderr, "%s(\"%s\")\n", __func__, app);
#endif
	if((appclient = object_new(sizeof(AppClient))) == NULL)
		return NULL;
	appclient->interface = appinterface_new_client(app);
	appclient->event = event;
	appclient->helper.data = appclient;
	appclient->helper.message = _appclient_helper_message;
	appclient->transport = apptransport_new(ATM_CLIENT, &appclient->helper,
			"tcp", "127.0.0.1:4250", event);
	if(appclient->interface == NULL || appclient->transport == NULL)
	{
		appclient_delete(appclient);
		return NULL;
	}
	return appclient;
}


/* appclient_delete */
void appclient_delete(AppClient * appclient)
{
	if(appclient->interface != NULL)
		appinterface_delete(appclient->interface);
	if(appclient->transport != NULL)
		apptransport_delete(appclient->transport);
	object_delete(appclient);
}


/* useful */
/* appclient_call */
int appclient_call(AppClient * client, int32_t * ret, char const * call, ...)
{
	va_list ap;
	AppMessage * message;
	int res;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, call);
#endif
	va_start(ap, call);
	message = appinterface_call(client->interface, call, ap);
	va_end(ap);
	if(message == NULL)
		return -1;
	res = apptransport_send(client->transport, message, 1);
	appmessage_delete(message);
	/* FIXME obtain the answer */
	if(ret != NULL)
		*ret = 0;
#ifdef DEBUG
	fprintf(stderr, "%s() => %d\n", __func__, res);
#endif
	return res;
}


/* private */
/* functions */
/* helper */
/* appclient_helper_message */
static int _appclient_helper_message(AppClient * appclient,
		AppTransport * transport, AppTransportClient * client,
		AppMessage * message)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* FIXME call the proper method (or return an error) */
	return 0;
}
