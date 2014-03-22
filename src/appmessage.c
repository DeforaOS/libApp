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



#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <System.h>
#include "appmessage.h"


/* AppMessage */
/* private */
/* types */
struct _AppMessage
{
	AppMessageType type;
	AppMessageID id;

	union
	{
		struct
		{
			char * method;
			AppMessageCallArgument * args;
			size_t args_cnt;
		} call;
	} t;
};


/* public */
/* functions */
/* appmessage_new_acknowledgement */
AppMessage * appmessage_new_acknowledgement(AppMessageID id)
{
	AppMessage * message;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, method);
#endif
	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_ACKNOWLEDGEMENT;
	message->id = id;
	return message;
}


/* appmessage_new_call */
AppMessage * appmessage_new_call(char const * method,
		AppMessageCallArgument * args, size_t args_cnt)
{
	AppMessage * message;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, method);
#endif
	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_CALL;
	message->id = 0;
	message->t.call.method = string_new(method);
	message->t.call.args = malloc(sizeof(*args) * args_cnt);
	message->t.call.args_cnt = args_cnt;
	if(message->t.call.args == NULL)
	{
		appmessage_delete(message);
		return NULL;
	}
	for(i = 0; i < args_cnt; i++)
	{
		/* FIXME check for errors */
		message->t.call.args[i].direction = args[i].direction;
		message->t.call.args[i].arg = variable_new_copy(args[i].arg);
	}
	return message;
}


/* appmessage_new_callv */
AppMessage * appmessage_new_callv(char const * method, ...)
{
	AppMessage * message;
	va_list ap;
	size_t i;
	int type;
	Variable * v;
	AppMessageCallArgument * p;

	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_CALL;
	message->id = 0;
	message->t.call.method = string_new(method);
	message->t.call.args = NULL;
	message->t.call.args_cnt = 0;
	/* check for errors */
	if(message->t.call.method == NULL)
	{
		appmessage_delete(message);
		return NULL;
	}
	/* copy the arguments */
	va_start(ap, method);
	for(i = 0; (type = va_arg(ap, int)) >= 0; i++)
	{
		if((p = realloc(message->t.call.args, sizeof(*p) * (i + 1)))
				== NULL)
		{
			appmessage_delete(message);
			message = NULL;
			break;
		}
		message->t.call.args = p;
		if((v = variable_new(type, va_arg(ap, void *))) == NULL)
		{
			appmessage_delete(message);
			message = NULL;
			break;
		}
		message->t.call.args[i].direction = AMCD_IN; /* XXX */
		message->t.call.args[i].arg = v;
		message->t.call.args_cnt = i + 1;
	}
	va_end(ap);
	return message;
}


/* appmessage_new_callv_variables */
AppMessage * appmessage_new_callv_variables(char const * method, ...)
{
	AppMessage * message;
	va_list ap;
	size_t i;
	Variable * v;
	AppMessageCallArgument * p;

	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->type = AMT_CALL;
	message->id = 0;
	message->t.call.method = string_new(method);
	message->t.call.args = NULL;
	message->t.call.args_cnt = 0;
	/* check for errors */
	if(message->t.call.method == NULL)
	{
		appmessage_delete(message);
		return NULL;
	}
	/* copy the arguments */
	va_start(ap, method);
	for(i = 0; (v = va_arg(ap, Variable *)) != NULL; i++)
	{
		if((p = realloc(message->t.call.args, sizeof(*p) * (i + 1)))
				== NULL)
			break;
		message->t.call.args = p;
		if((v = variable_new_copy(v)) == NULL)
		{
			appmessage_delete(message);
			message = NULL;
			break;
		}
		message->t.call.args[i].direction = AMCD_IN; /* XXX */
		message->t.call.args[i].arg = v;
		message->t.call.args_cnt = i + 1;
	}
	va_end(ap);
	return message;
}


/* appmessage_new_deserialize */
static AppMessage * _new_deserialize_acknowledgement(AppMessage * message,
		char const * data, const size_t size, size_t pos);
static AppMessage * _new_deserialize_call(AppMessage * message,
		char const * data, const size_t size, size_t pos);
static AppMessage * _new_deserialize_id(AppMessage * message, char const * data,
		const size_t size, size_t * pos);

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
		case AMT_ACKNOWLEDGEMENT:
			return _new_deserialize_acknowledgement(message, data,
					size, pos);
		case AMT_CALL:
			return _new_deserialize_call(message, data, size, pos);
		default:
			error_set_code(1, "%s%u", "Unknown message type ", u8);
			/* XXX should not happen */
			object_delete(message);
			return NULL;
	}
}

static AppMessage * _new_deserialize_acknowledgement(AppMessage * message,
		char const * data, const size_t size, size_t pos)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	return _new_deserialize_id(message, data, size, &pos);
}

static AppMessage * _new_deserialize_call(AppMessage * message,
		char const * data, const size_t size, size_t pos)
{
	size_t s;
	Variable * v;
	size_t i;
	AppMessageCallArgument * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(_new_deserialize_id(message, data, size, &pos) == NULL)
		return NULL;
	message->t.call.method = NULL;
	message->t.call.args = NULL;
	message->t.call.args_cnt = 0;
	s = size;
	if((v = variable_new_deserialize_type(VT_STRING, &s, &data[pos]))
			== NULL)
	{
		error_set_code(1, "%s", "Could not obtain the AppMessage call"
				" method");
		appmessage_delete(message);
		return NULL;
	}
	pos += s;
	/* XXX may fail */
	variable_get_as(v, VT_STRING, &message->t.call.method);
	variable_delete(v);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__,
			message->t.call.method);
#endif
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
		s = size - pos;
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
		message->t.call.args[i].direction = AMCD_IN; /* XXX */
		message->t.call.args[i].arg = v;
		message->t.call.args_cnt = i + 1;
	}
	return message;
}

static AppMessage * _new_deserialize_id(AppMessage * message, char const * data,
		const size_t size, size_t * pos)
{
	int ret = 0;
	size_t s = size;
	Variable * v;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((v = variable_new_deserialize_type(VT_UINT32, &s, &data[*pos]))
			== NULL)
	{
		error_set_code(1, "%s", "Could not obtain the AppMessage ID");
		appmessage_delete(message);
		return NULL;
	}
	ret = variable_get_as(v, VT_UINT32, &message->id);
	variable_delete(v);
	if(ret != 0)
	{
		appmessage_delete(message);
		return NULL;
	}
	*pos += s;
	return message;
}


/* appmessage_delete */
static void _delete_call(AppMessage * message);

void appmessage_delete(AppMessage * message)
{
	switch(message->type)
	{
		case AMT_ACKNOWLEDGEMENT:
			break;
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
/* appmessage_get_id */
AppMessageID appmessage_get_id(AppMessage * message)
{
	return message->id;
}


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


/* appmessage_set_id */
void appmessage_set_id(AppMessage * message, AppMessageID id)
{
	message->id = id;
}


/* useful */
/* appmessage_serialize */
static int _serialize_acknowledgement(AppMessage * message, Buffer * buffer,
		Buffer * b);
static int _serialize_append(Buffer * buffer, Buffer * b);
static int _serialize_call(AppMessage * message, Buffer * buffer, Buffer * b);
static int _serialize_id(AppMessage * message, Buffer * buffer, Buffer * b);
static int _serialize_type(AppMessage * message, Buffer * buffer, Buffer * b);

int appmessage_serialize(AppMessage * message, Buffer * buffer)
{
	Buffer * b;

	if((b = buffer_new(0, NULL)) == NULL)
		return -1;
	/* reset the output buffer */
	buffer_set_size(buffer, 0);
	if(_serialize_type(message, buffer, b) != 0)
		return -1;
	switch(message->type)
	{
		case AMT_ACKNOWLEDGEMENT:
			return _serialize_acknowledgement(message, buffer, b);
		case AMT_CALL:
			return _serialize_call(message, buffer, b);
		default:
			return -error_set_code(1, "%s%u",
					"Unable to serialize message type ",
					message->type);
	}
}

static int _serialize_acknowledgement(AppMessage * message, Buffer * buffer,
		Buffer * b)
{
	return _serialize_id(message, buffer, b);
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

	if(_serialize_id(message, buffer, b) != 0)
		return -1;
	if((v = variable_new(VT_STRING, message->t.call.method)) == NULL)
		return -1;
	ret = (variable_serialize(v, b, 0) == 0
			&& _serialize_append(buffer, b) == 0) ? 0 : -1;
	variable_delete(v);
	if(ret != 0)
		return ret;
	for(i = 0; i < message->t.call.args_cnt; i++)
	{
		if(message->t.call.args[i].direction == AMCD_OUT)
			continue;
		if(variable_serialize(message->t.call.args[i].arg, b, 1) != 0
				|| _serialize_append(buffer, b) != 0)
			break;
	}
	buffer_delete(b);
	return (i == message->t.call.args_cnt) ? 0 : -1;
}

static int _serialize_id(AppMessage * message, Buffer * buffer, Buffer * b)
{
	int ret;
	Variable * v;

	if((v = variable_new(VT_UINT32, &message->id)) == NULL)
		return -1;
	ret = (variable_serialize(v, b, 0) == 0
			&& _serialize_append(buffer, b) == 0) ? 0 : -1;
	variable_delete(v);
	return ret;
}

static int _serialize_type(AppMessage * message, Buffer * buffer, Buffer * b)
{
	int ret;
	Variable * v;

	if((v = variable_new(VT_UINT8, &message->type)) == NULL)
		return -1;
	ret = (variable_serialize(v, b, 0) == 0
			&& _serialize_append(buffer, b) == 0) ? 0 : -1;
	variable_delete(v);
	return ret;
}
