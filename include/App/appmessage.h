/* $Id$ */
/* Copyright (c) 2012-2020 Pierre Pronchery <khorben@defora.org> */
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



#ifndef LIBAPP_APP_APPMESSAGE_H
# define LIBAPP_APP_APPMESSAGE_H

# include <System/buffer.h>
# include <System/string.h>
# include <System/variable.h>
# include <System/Marshall.h>
# include "app.h"
# include "appstatus.h"


/* AppMessage */
/* types */
typedef enum _AppMessageType
{
	AMT_STATUS_GET = 0,
	AMT_STATUS_SET,
	AMT_CALL,
	AMT_ACKNOWLEDGEMENT
} AppMessageType;
# define AMT_CALLBACK	AMT_CALL

typedef uint32_t AppMessageID;

/* calls */
typedef enum _AppMessageCallDirection
{
	AMCD_IN = MCD_IN,
	AMCD_OUT = MCD_OUT,
	AMCD_IN_OUT = MCD_IN_OUT
} AppMessageCallDirection;

typedef struct _AppMessageCallArgument
{
	AppMessageCallDirection direction;
	Variable * arg;
} AppMessageCallArgument;


/* macros */
# define AMCA(type, direction, variable) \
	(type | (direction) << 8), (variable)


/* functions */
AppMessage * appmessage_new_deserialize(Buffer * buffer);
/* calls */
AppMessage * appmessage_new_call(String const * method,
		AppMessageCallArgument * args, size_t args_cnt);
AppMessage * appmessage_new_callv(String const * method, ...);
AppMessage * appmessage_new_callv_variables(String const * method, ...);
/* status */
AppMessage * appmessage_new_status(AppStatus * status);

void appmessage_delete(AppMessage * appmessage);

/* accessors */
String const * appmessage_get_method(AppMessage * message);
AppMessageType appmessage_get_type(AppMessage * message);

/* useful */
int appmessage_serialize(AppMessage * message, Buffer * buffer);

#endif /* !LIBAPP_APP_APPMESSAGE_H */
