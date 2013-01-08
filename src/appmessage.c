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



#include <stdarg.h>
#include <stddef.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <System.h>
#include "App/appmessage.h"


/* AppMessage */
/* private */
/* types */
struct _AppMessage
{
	AppMessageType type;

	union
	{
		struct
		{
			char * method;
			Variable ** var;
			size_t var_cnt;
		} call;
	} t;
};


/* private */
/* prototypes */


/* public */
/* functions */
/* appmessage_new_call */
AppMessage * appmessage_new_call(char const * method, ...)
{
	AppMessage * message;
	va_list ap;
	Variable * v;

	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_CALL;
	message->t.call.method = string_new(method);
	/* copy the arguments */
	va_start(ap, method);
	while((v = va_arg(ap, Variable *)) != NULL)
		;
	va_end(ap);
	/* check for errors */
	if(message->t.call.method == NULL)
	{
		appmessage_delete(message);
		return NULL;
	}
	return message;
}


/* appmessage_new_deserialize */
AppMessage * appmessage_new_deserialize(Buffer * buffer)
{
	AppMessage * message;
	char const * data = buffer_get_data(buffer);
	size_t size = buffer_get_size(buffer);
	size_t pos = 0;
	size_t s;
	Variable * v;
	uint8_t u8;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	s = size;
	if((v = variable_new_deserialize_type(VT_UINT8, &s, &data[pos]))
			== NULL)
	{
		object_delete(message);
		return NULL;
	}
	pos += s;
	size -= s;
	/* XXX may fail */
	variable_get_as(v, VT_UINT8, &u8);
	variable_delete(v);
	switch((message->type = u8))
	{
		case AMT_CALL:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() AMT_CALL\n", __func__);
#endif
			s = size;
			v = variable_new_deserialize_type(VT_STRING, &s,
					&data[pos]);
			if(v == NULL)
			{
				error_set_code(1, "%s%u", "Unknown method ");
				object_delete(message);
				return NULL;
			}
			/* XXX may fail */
			variable_get_as(v, VT_STRING, &message->t.call.method);
			variable_delete(v);
			/* FIXME really implement */
			message->t.call.var = NULL;
			message->t.call.var_cnt = 0;
			break;
		default:
			error_set_code(1, "%s%u", "Unknown message type ", u8);
			/* XXX should not happen */
			object_delete(message);
			return NULL;
	}
	return message;
}


/* appmessage_delete */
static void _delete_call(AppMessage * message);

void appmessage_delete(AppMessage * message)
{
	switch(message->type)
	{
		case AMT_CALL:
			_delete_call(message);
			break;
	}
	object_delete(message);
}

static void _delete_call(AppMessage * message)
{
	string_delete(message->t.call.method);
}


/* accessors */
/* appmessage_get_method */
String const * appmessage_get_method(AppMessage * message)
{
	if(message->type == AMT_CALL)
		return message->t.call.method;
	return NULL;
}


/* appmessage_get_type */
AppMessageType appmessage_get_type(AppMessage * message)
{
	return message->type;
}


/* useful */
/* appmessage_serialize */
int appmessage_serialize(AppMessage * message, Buffer * buffer)
{
	int ret;
	Variable * v;

	if((v = variable_new(VT_UINT8, &message->type)) == NULL)
		return -1;
	/* FIXME really implement */
	ret = variable_serialize(v, buffer, 0);
	variable_delete(v);
	return ret;
}
