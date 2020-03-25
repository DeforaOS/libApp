/* $Id$ */
/* Copyright (c) 2011-2020 Pierre Pronchery <khorben@defora.org> */
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



#ifndef LIBAPP_APPINTERFACE_H
# define LIBAPP_APPINTERFACE_H

# include <stdarg.h>
# include <System/variable.h>
# include "App/appstatus.h"
# include "App/apptransport.h"


/* AppInterface */
/* types */
typedef struct _AppInterface AppInterface;


/* functions */
AppInterface * appinterface_new(AppTransportMode mode, String const * app);
AppInterface * appinterface_new_interface(AppTransportMode mode,
		String const * app, String const * pathname);
void appinterface_delete(AppInterface * appinterface);

/* accessors */
int appinterface_can_call(AppInterface * appinterface, char const * method,
		char const * name);

char const * appinterface_get_app(AppInterface * appinterface);
int appinterface_get_args_count(AppInterface * appinterface, size_t * count,
		char const * function);
AppStatus * appinterface_get_status(AppInterface * appinterface);

/* useful */
int appinterface_callv(AppInterface * appinterface, App * app, void ** result,
		char const * method, va_list args);
int appinterface_call_variablev(AppInterface * appinterface, App * app,
		Variable * result, char const * method,
		size_t argc, Variable ** argv);

AppMessage * appinterface_messagev(AppInterface * appinterface,
		char const * method, va_list args);
AppMessage * appinterface_message_variables(AppInterface * appinterface,
		char const * method, Variable ** args);
AppMessage * appinterface_message_variablev(AppInterface * appinterface,
		char const * method, va_list args);

#endif /* !LIBAPP_APPINTERFACE_H */
