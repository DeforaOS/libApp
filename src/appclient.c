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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App/appclient.h"
#include "App/appmessage.h"
#include "apptransport.h"
#include "appinterface.h"


/* AppClient */
/* private */
/* types */
struct _AppClient
{
	App * app;
	AppInterface * interface;
	Event * event;
	int event_free;
	AppTransport * transport;
	AppTransportHelper helper;
};


/* prototypes */
/* helpers */
static int _appclient_helper_message(void * data, AppTransport * transport,
		AppTransportClient * client, AppMessage * message);


/* public */
/* functions */
/* appclient_new */
AppClient * appclient_new(App * self, char const * app, char const * name)
{
	return appclient_new_event(self, app, name, NULL);
}


/* appclient_new_event */
AppClient * appclient_new_event(App * self, char const * app,
		char const * name, Event * event)
{
	AppClient * appclient;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", \"%s\", %p)\n", __func__, app, name,
			(void *)event);
#endif
	if((appclient = object_new(sizeof(*appclient))) == NULL)
		return NULL;
	appclient->app = self;
	appclient->interface = appinterface_new(ATM_CLIENT, app);
	appclient->helper.data = appclient;
	appclient->helper.message = _appclient_helper_message;
	appclient->event = (event != NULL) ? event : event_new();
	appclient->event_free = (event != NULL) ? 0 : 1;
	appclient->transport = apptransport_new_app(ATM_CLIENT,
			&appclient->helper, app, name, appclient->event);
	/* check for errors */
	if(appclient->interface == NULL || appclient->transport == NULL
			|| appclient->event == NULL)
	{
		appclient_delete(appclient);
		return NULL;
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %p) => %p\n", __func__, app,
			(void *)event, (void *)appclient);
#endif
	return appclient;
}


/* appclient_delete */
void appclient_delete(AppClient * appclient)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(appclient->interface != NULL)
		appinterface_delete(appclient->interface);
	if(appclient->event_free != 0)
		event_delete(appclient->event);
	object_delete(appclient);
}


/* accessors */
/* appclient_get_app */
char const * appclient_get_app(AppClient * appclient)
{
	return appinterface_get_app(appclient->interface);
}


/* useful */
/* appclient_call */
int appclient_call(AppClient * appclient,
		void ** result, char const * method, ...)
{
	int ret;
	va_list ap;

	va_start(ap, method);
	ret = appinterface_callv(appclient->interface, appclient->app, result,
			method, ap);
	va_end(ap);
	return ret;
}


/* appclient_call_variable */
int appclient_call_variable(AppClient * appclient,
		Variable * result, char const * method, ...)
{
	int ret;
	size_t cnt;
	size_t i;
	va_list ap;
	Variable ** argv;

	if(appinterface_get_args_count(appclient->interface, &cnt, method) != 0)
		return -1;
	if((argv = malloc(sizeof(*argv) * cnt)) == NULL)
		return error_set_code(-errno, "%s", strerror(errno));
	va_start(ap, method);
	for(i = 0; i < cnt; i++)
		argv[i] = va_arg(ap, Variable *);
	va_end(ap);
	ret = appinterface_call_variablev(appclient->interface, appclient->app,
			result, method, cnt, argv);
	free(argv);
	return ret;
}


/* private */
/* appclient_helper_message */
static int _helper_message_call(AppClient * appclient, AppTransport * transport,
		AppMessage * message);

static int _appclient_helper_message(void * data, AppTransport * transport,
		AppTransportClient * client, AppMessage * message)
{
	AppClient * appclient = data;

	if(client != NULL)
		/* XXX report error */
		return -1;
	switch(appmessage_get_type(message))
	{
		case AMT_CALL:
			return _helper_message_call(appclient, transport,
					message);
	}
	/* FIXME implement */
	return -1;
}

static int _helper_message_call(AppClient * appclient, AppTransport * transport,
		AppMessage * message)
{
	/* we have received a callback request */
	int ret;
	String const * method;
	Variable * result = NULL;

	method = appmessage_get_method(message);
	if(!appinterface_can_call(appclient->interface, NULL, method))
		/* XXX report errors */
		return -1;
	ret = appinterface_call_variablev(appclient->interface, appclient->app,
			result, method, 0, NULL);
	if(result != NULL)
		variable_delete(result);
	return ret;
}
