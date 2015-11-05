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



#ifndef LIBAPP_APP_APPSERVER_H
# define LIBAPP_APP_APPSERVER_H

# include <System/event.h>
# include "app.h"
# include "appstatus.h"


/* AppServer */
/* types */
typedef void AppServerClient;

typedef unsigned int AppServerOptions;
# define ASO_REGISTER	0x1

typedef struct _AppServer AppServer;


/* constants */
# define APPSERVER_MAX_ARGUMENTS	4


/* functions */
AppServer * appserver_new(App * self, AppServerOptions options,
		char const * app, char const * name);
AppServer * appserver_new_event(App * self, AppServerOptions options,
		char const * app, char const * name, Event * event);
void appserver_delete(AppServer * appserver);

/* accessors */
char const * appserver_get_app(AppServer * appserver);
AppStatus * appserver_get_status(AppServer * appserver);

/* useful */
int appserver_loop(AppServer * appserver);
int appserver_register(AppServer * appserver, char const * name);

#endif /* !LIBAPP_APP_APPSERVER_H */
