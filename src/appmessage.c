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
	size_t s;
	Variable * v;
	uint32_t u32;

	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	s = size;
	if((v = variable_new_deserialize_type(VT_UINT32, &s, data)) == NULL)
	{
		object_delete(message);
		return NULL;
	}
	/* XXX may fail */
	variable_get_as(v, VT_UINT32, &u32);
	variable_delete(v);
	switch((message->type = u32))
	{
		case AMT_CALL:
			/* FIXME really implement */
			message->t.call.method = NULL;
			message->t.call.var = NULL;
			message->t.call.var_cnt = 0;
			break;
		default:
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
