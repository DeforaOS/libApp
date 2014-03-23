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



#ifndef LIBAPP_APPTRANSPORT_H
# define LIBAPP_APPTRANSPORT_H

# include "App/apptransport.h"


/* AppTransport */
/* protected */
/* types */
typedef struct _AppTransportHelper
{
	void * data;
	void (*message)(void * data, AppTransport * transport,
			AppTransportClient * client, AppMessage * message);
} AppTransportHelper;


/* functions */
AppTransport * apptransport_new(AppTransportMode mode,
		AppTransportHelper * helper, char const * plugin,
		char const * name, Event * event);
void apptransport_delete(AppTransport * transport);

/* useful */
/* ATM_CLIENT */
int apptransport_client_send(AppTransport * transport, AppMessage * message,
		int acknowledge);

/* ATM_SERVER */
int apptransport_server_send(AppTransport * transport,
		AppTransportClient * client, AppMessage * message);

#endif /* !LIBAPP_APPTRANSPORT_H */
