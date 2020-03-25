/* $Id$ */
/* Copyright (c) 2011-2015 Pierre Pronchery <khorben@defora.org> */
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



#ifndef LIBAPP_APP_APPCLIENT_H
# define LIBAPP_APP_APPCLIENT_H

# include <stdarg.h>
# include <stdint.h>
# include <System/event.h>
# include <System/variable.h>
# include "app.h"
# include "appstatus.h"


/* AppClient */
/* types */
typedef struct _AppClient AppClient;


/* functions */
AppClient * appclient_new(App * self, char const * app, char const * name);
AppClient * appclient_new_event(App * self, char const * app,
		char const * name, Event * event);
void appclient_delete(AppClient * appclient);

/* accessors */
AppStatus * appclient_get_status(AppClient * appclient);

/* useful */
int appclient_call(AppClient * appclient,
		void ** result, char const * method, ...);
int appclient_callv(AppClient * appclient,
		void ** result, char const * method, va_list args);
int appclient_call_variable(AppClient * appclient,
		Variable * result, char const * method, ...);
int appclient_call_variables(AppClient * appclient,
		Variable * result, char const * method, Variable ** args);
int appclient_call_variablev(AppClient * appclient,
		Variable * result, char const * method, va_list args);

#endif /* !LIBAPP_APP_APPCLIENT_H */
