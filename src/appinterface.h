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



#ifndef LIBAPP_APPINTERFACE_H
# define LIBAPP_APPINTERFACE_H

# include <stdarg.h>
# include <System/variable.h>


/* AppInterface */
/* types */
typedef struct _AppInterface AppInterface;


/* functions */
AppInterface * appinterface_new(char const * app);
AppInterface * appinterface_new_server(char const * app);
void appinterface_delete(AppInterface * appinterface);

/* accessors */
int appinterface_can_call(AppInterface * appinterface, char const * name,
		char const * method);

char const * appinterface_get_app(AppInterface * appinterface);
int appinterface_get_args_count(AppInterface * appinterface, size_t * count,
		char const * function);

/* useful */
int appinterface_callv(AppInterface * appinterface, void ** result,
		char const * method, va_list args);
int appinterface_call_variablev(AppInterface * appinterface, Variable * result,
		char const * method, size_t argc, Variable ** argv);

#endif /* !LIBAPP_APPINTERFACE_H */
