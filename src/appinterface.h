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



#ifndef LIBAPP_APPINTERFACE2_H
# define LIBAPP_APPINTERFACE2_H

# include <stdarg.h>
# include "App/app.h"


/* AppInterface */
/* types */
typedef struct _AppInterface AppInterface;


/* constants */
# define APPINTERFACE_MAX_ARGUMENTS	4


/* functions */
AppInterface * appinterface2_new_client(char const * app);
AppInterface * appinterface2_new_server(char const * app);
void appinterface2_delete(AppInterface * interface);

/* useful */
AppMessage * appinterface2_call(AppInterface * interface, char const * call,
		va_list args);
int appinterface2_call_process(AppInterface * interface, AppMessage * message);

#endif /* !LIBAPP_APPINTERFACE2_H */
