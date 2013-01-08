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
#include <stdlib.h>
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
			Variable ** args;
			size_t args_cnt;
		} call;
	} t;
};


/* public */
/* functions */
/* appmessage_new_call */
AppMessage * appmessage_new_call(char const * method, ...)
{
	AppMessage * message;
	va_list ap;
	size_t i;
	int type;
	Variable * v;
	Variable ** p;

	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_CALL;
	message->t.call.method = string_new(method);
	message->t.call.args = NULL;
	message->t.call.args_cnt = 0;
	/* copy the arguments */
	va_start(ap, method);
	for(i = 0; (type = va_arg(ap, int)) >= 0; i++)
	{
		if((p = realloc(message->t.call.args, sizeof(*p) * (i + 1)))
				== NULL)
			break;
		message->t.call.args = p;
		if((v = variable_new(type, va_arg(ap, void *))) == NULL)
			break;
		message->t.call.args = p;
		message->t.call.args[i] = v;
		message->t.call.args_cnt = i + 1;
	}
	va_end(ap);
	/* check for errors */
	if(message->t.call.method == NULL || type >= 0)
	{
		appmessage_delete(message);
		return NULL;
	}
	return message;
}


/* appmessage_new_deserialize */
static AppMessage * _new_deserialize_call(AppMessage * message,
		char const * data, size_t size, size_t pos);

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
			return _new_deserialize_call(message, data, size, pos);
		default:
			error_set_code(1, "%s%u", "Unknown message type ", u8);
			/* XXX should not happen */
			object_delete(message);
			return NULL;
	}
}

static AppMessage * _new_deserialize_call(AppMessage * message,
		char const * data, size_t size, size_t pos)
{
	size_t s;
	Variable * v;
	size_t i;
	Variable ** p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	message->t.call.method = NULL;
	message->t.call.args = NULL;
	message->t.call.args_cnt = 0;
	s = size;
	if((v = variable_new_deserialize_type(VT_STRING, &s, &data[pos]))
			== NULL)
	{
		error_set_code(1, "%s%u", "Unknown method ");
		appmessage_delete(message);
		return NULL;
	}
	pos += s;
	size -= s;
	/* XXX may fail */
	variable_get_as(v, VT_STRING, &message->t.call.method);
	variable_delete(v);
	/* deserialize the arguments */
	for(i = 0; pos < size; i++)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %lu\n", __func__, i);
#endif
		if((p = realloc(message->t.call.args, sizeof(*p) * (i + 1)))
				== NULL)
		{
			appmessage_delete(message);
			return NULL;
		}
		message->t.call.args = p;
		s = size;
		if((v = variable_new_deserialize(&s, &data[pos])) == NULL)
		{
			appmessage_delete(message);
			return NULL;
		}
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %lu (%u)\n", __func__, i,
				variable_get_type(v));
#endif
		pos += s;
		size -= s;
		message->t.call.args[i] = v;
		message->t.call.args_cnt = i + 1;
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
static int _serialize_append(Buffer * buffer, Buffer * b);
static int _serialize_call(AppMessage * message, Buffer * buffer, Buffer * b);

int appmessage_serialize(AppMessage * message, Buffer * buffer)
{
	int ret;
	Variable * v;
	Buffer * b;

	if((b = buffer_new(0, NULL)) == NULL)
		return -1;
	/* reset the output buffer */
	buffer_set_size(buffer, 0);
	if((v = variable_new(VT_UINT8, &message->type)) == NULL)
		return -1;
	ret = (variable_serialize(v, b, 0) == 0
			&& _serialize_append(buffer, b) == 0) ? 0 : -1;
	variable_delete(v);
	if(ret != 0)
		return ret;
	switch(message->type)
	{
		case AMT_CALL:
			return _serialize_call(message, buffer, b);
		default:
			return -error_set_code(1, "%s%u",
					"Unable to serialize message type ",
					message->type);
	}
}

static int _serialize_append(Buffer * buffer, Buffer * b)
{
	return buffer_set_data(buffer, buffer_get_size(buffer),
			buffer_get_data(b), buffer_get_size(b));
}

static int _serialize_call(AppMessage * message, Buffer * buffer, Buffer * b)
{
	int ret;
	Variable * v;
	size_t i;

	if((v = variable_new(VT_STRING, message->t.call.method)) == NULL)
		return -1;
	ret = (variable_serialize(v, b, 0) == 0
			&& _serialize_append(buffer, b) == 0) ? 0 : -1;
	variable_delete(v);
	if(ret != 0)
		return ret;
	for(i = 0; i < message->t.call.args_cnt; i++)
		if(variable_serialize(message->t.call.args[i], b, 1) != 0
				|| _serialize_append(buffer, b) != 0)
			break;
	buffer_delete(b);
	return (i == message->t.call.args_cnt) ? 0 : -1;
}
