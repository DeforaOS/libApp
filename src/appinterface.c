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



#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WIN32__
# include <Winsock2.h>
#else
# include <arpa/inet.h>
# include <netinet/in.h>
#endif
#include <errno.h>
#include <System.h>
#include "App/appmessage.h"
#include "appinterface.h"
#include "../config.h"

#ifndef PREFIX
# define PREFIX	"/usr/local"
#endif
#ifndef ETCDIR
# define ETCDIR	PREFIX "/etc"
#endif


/* AppInterface */
/* private */
/* types */
typedef struct _AppInterfaceCallArg
{
	VariableType type;
	AppMessageCallDirection direction;
} AppInterfaceCallArg;

typedef struct _AppInterfaceCall
{
	char * name;
	AppInterfaceCallArg type;
	AppInterfaceCallArg * args;
	size_t args_cnt;
	void * func;
} AppInterfaceCall;

struct _AppInterface
{
	String * name;
	String * prefix;
	AppInterfaceCall * calls;
	size_t calls_cnt;
	int error;
};

typedef struct _StringEnum
{
	char const * string;
	int type;
	int direction;
} StringEnum;


/* variables */
static StringEnum _string_type[] =
{
	{ "VOID",	VT_NULL,	0		},
	{ "BOOL",	VT_BOOL,	AMCD_IN		},
	{ "INT8",	VT_INT8,	AMCD_IN		},
	{ "UINT8",	VT_UINT8,	AMCD_IN		},
	{ "INT16",	VT_INT16,	AMCD_IN		},
	{ "UINT16",	VT_UINT16,	AMCD_IN		},
	{ "INT32",	VT_INT32,	AMCD_IN		},
	{ "UINT32",	VT_UINT32,	AMCD_IN		},
	{ "INT64",	VT_INT64,	AMCD_IN		},
	{ "UINT64",	VT_UINT64,	AMCD_IN		},
	{ "STRING",	VT_STRING,	AMCD_IN		},
	{ "STRING_OUT",	VT_STRING,	AMCD_OUT	},
	{ "BUFFER",	VT_BUFFER,	AMCD_IN		},
	{ "BUFFER_OUT",	VT_BUFFER,	AMCD_OUT	},
	{ "FLOAT",	VT_FLOAT,	AMCD_IN		},
	{ "DOUBLE",	VT_DOUBLE,	AMCD_IN		},
	{ NULL,		0,		0		}
};


/* prototypes */
static AppInterface * _appinterface_new(char const * app, char const * prefix);

/* accessors */
static AppInterfaceCall * _appinterface_get_call(AppInterface * appinterface,
		char const * call);

static int _string_enum(StringEnum * se, String const * string,
		VariableType * type, AppMessageCallDirection * direction);


/* public */
/* functions */
/* appinterface_new_client */
AppInterface * appinterface_new_client(char const * app)
{
	/* FIXME also lookup the callbacks */
	return _appinterface_new(app, "call");
}


/* appinterface_new_server */
AppInterface * appinterface_new_server(char const * app)
{
	AppInterface * ai;
	Plugin * handle;
	size_t i;
	String * name;

	if((ai = _appinterface_new(app, "call")) == NULL)
		return NULL;
	if((handle = plugin_new_self()) == NULL)
	{
		appinterface_delete(ai);
		return NULL;
	}
	for(i = 0; i < ai->calls_cnt; i++)
	{
		name = string_new_append(ai->name, "_", ai->calls[i].name,
				NULL);
		ai->calls[i].func = plugin_lookup(handle, name);
		string_delete(name);
		if(ai->calls[i].func == NULL)
		{
			appinterface_delete(ai);
			plugin_delete(handle);
			return NULL;
		}
	}
	plugin_delete(handle);
	return ai;
}


/* appinterface_delete */
void appinterface_delete(AppInterface * appinterface)
{
	size_t i;

	for(i = 0; i < appinterface->calls_cnt; i++)
	{
		free(appinterface->calls[i].name);
		free(appinterface->calls[i].args);
	}
	free(appinterface->calls);
	string_delete(appinterface->prefix);
	string_delete(appinterface->name);
	object_delete(appinterface);
}


/* useful */
/* appinterface_call */
AppMessage * appinterface_call(AppInterface * interface, char const * method,
		va_list args)
{
	AppInterfaceCall * call;
	AppMessageCallArgument * callargs;
	size_t i;
	size_t j;
	uint8_t i8;
	uint8_t u8;
	uint16_t i16;
	uint16_t u16;
	uint32_t i32;
	uint32_t u32;
	uint64_t i64;
	uint64_t u64;
	float f;
	double d;
	String * s;
	Buffer * b;
	Variable * v;
	AppMessage * message;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, method);
#endif
	if((call = _appinterface_get_call(interface, method)) == NULL)
		return NULL;
	if((callargs = malloc(sizeof(*args) * call->args_cnt)) == NULL)
	{
		error_set_code(1, "%s", strerror(errno));
		return NULL;
	}
	for(i = 0; i < call->args_cnt; i++)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() arg %lu (%u)\n", __func__, i,
				call->args[i].type);
#endif
		if((callargs[i].direction = call->args[i].direction)
				== AMCD_OUT)
		{
			callargs[i].arg = NULL;
			continue;
		}
		/* FIXME assumes AMCD_IN from here on */
		v = NULL;
		switch(call->args[i].type)
		{
			case VT_NULL:
				break;
			case VT_BOOL:
			case VT_UINT8:
				u8 = va_arg(args, unsigned int);
				v = variable_new(call->args[i].type, &u8);
				break;
			case VT_INT8:
				i8 = va_arg(args, int);
				v = variable_new(call->args[i].type, &i8);
				break;
			case VT_INT16:
				i16 = va_arg(args, int);
				v = variable_new(call->args[i].type, &i16);
				break;
			case VT_UINT16:
				u16 = va_arg(args, unsigned int);
				v = variable_new(call->args[i].type, &u16);
				break;
			case VT_INT32:
				i32 = va_arg(args, int32_t);
				v = variable_new(call->args[i].type, &i32);
				break;
			case VT_UINT32:
				u32 = va_arg(args, uint32_t);
				v = variable_new(call->args[i].type, &u32);
				break;
			case VT_INT64:
				i64 = va_arg(args, int64_t);
				v = variable_new(call->args[i].type, &i64);
				break;
			case VT_UINT64:
				u64 = va_arg(args, uint64_t);
				v = variable_new(call->args[i].type, &u64);
				break;
			case VT_FLOAT:
				f = va_arg(args, double);
				v = variable_new(call->args[i].type, &f);
				break;
			case VT_DOUBLE:
				d = va_arg(args, double);
				v = variable_new(call->args[i].type, &d);
				break;
			case VT_STRING:
				s = va_arg(args, char *);
				v = variable_new(call->args[i].type, s);
				break;
			case VT_BUFFER:
				b = va_arg(args, Buffer *);
				v = variable_new(call->args[i].type, b);
				break;
		}
		if(v == NULL)
			break;
		callargs[i].arg = v;
	}
	if(i != call->args_cnt
			|| (message = appmessage_new_call(method, callargs,
					call->args_cnt)) == NULL)
	{
		for(j = 0; j < i; j++)
			variable_delete(callargs[j].arg);
		free(callargs);
		return NULL;
	}
	return message;
}


/* appinterface_call_process */
static int _call_process_exec(AppInterfaceCall * call, int32_t * ret,
		void ** args);

int appinterface_call_process(AppInterface * interface, AppMessage * message)
{
	AppInterfaceCall * call;
	int32_t ret = 0;
	void ** args;
	size_t i;
	AppMessageCallArgument * arg;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__,
			appmessage_get_method(message));
#endif
	if((call = _appinterface_get_call(interface, appmessage_get_method(
						message))) == NULL)
		return -1;
	if((args = malloc(sizeof(*args) * call->args_cnt)) == NULL)
		return -error_set_code(1, "%s", strerror(errno));
	for(i = 0; i < call->args_cnt; i++)
	{
		arg = appmessage_get_call_argument(message, i);
		if(arg->arg == NULL)
			/* XXX may be an AMCD_OUT or AMCD_IN_OUT */
			args[i] = NULL;
		else
			/* XXX may fail */
			variable_get_as(arg->arg, call->args[i].type, &args[i]);
	}
	_call_process_exec(call, &ret, args);
	for(i = 0; i < call->args_cnt; i++)
		switch(call->args[i].type)
		{
			case VT_STRING:
				string_delete(args[i]);
				break;
			case VT_BUFFER:
				buffer_delete(args[i]);
				break;
			default:
				break;
		}
	free(args);
	return 0;
}

static int _call_process_exec(AppInterfaceCall * call, int32_t * ret,
		void ** args)
{
	int (*func0)(void);
	int (*func1)(void *);
	int (*func2)(void *, void *);
	int (*func3)(void *, void *, void *);
	int (*func4)(void *, void *, void *, void *);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %lu\n", __func__, call->args_cnt);
#endif
	switch(call->args_cnt)
	{
		case 0:
			func0 = call->func;
			*ret = func0();
			break;
		case 1:
			func1 = call->func;
			*ret = func1(args[0]);
			break;
		case 2:
			func2 = call->func;
			*ret = func2(args[0], args[1]);
			break;
		case 3:
			func3 = call->func;
			*ret = func3(args[0], args[1], args[2]);
			break;
		case 4:
			func4 = call->func;
			*ret = func4(args[0], args[1], args[2], args[3]);
			break;
		default:
			return -error_set_code(1, "%lu%s", "methods with ",
					(unsigned long)call->args_cnt,
					"arguments are not supported");
	}
	if(call->type.type == VT_NULL) /* avoid information leak */
		*ret = 0;
	return 0;
}


/* private */
/* functions */
/* appinterface_new */
static int _new_foreach(char const * key, Hash * value,
		AppInterface * appinterface);
static int _new_append(AppInterface * ai, VariableType type,
		AppMessageCallDirection direction, char const * call);
static int _new_append_arg(AppInterface * ai, char const * arg);

static AppInterface * _appinterface_new(char const * app, char const * prefix)
{
	AppInterface * appinterface;
	String * pathname = NULL;
	Config * config = NULL;

	if(app == NULL)
		return NULL; /* FIXME report error */
	if((appinterface = object_new(sizeof(*appinterface))) == NULL)
		return NULL;
	appinterface->name = string_new(app);
	appinterface->prefix = string_new_append(prefix, "::", NULL);
	appinterface->calls = NULL;
	appinterface->calls_cnt = 0;
	appinterface->error = 0;
	if(appinterface->name == NULL
			|| appinterface->prefix == NULL
			|| (pathname = string_new_append(
					ETCDIR "/AppInterface/", app,
					".interface", NULL)) == NULL
			|| (config = config_new()) == NULL
			|| config_load(config, pathname) != 0)
	{
		if(config != NULL)
			config_delete(config);
		string_delete(pathname);
		appinterface_delete(appinterface);
		return NULL;
	}
	appinterface->error = 0;
	hash_foreach(config, (HashForeach)_new_foreach, appinterface);
	if(appinterface->error != 0)
	{
		appinterface_delete(appinterface);
		return NULL;
	}
	config_delete(config);
	return appinterface;
}

static int _new_foreach(char const * key, Hash * value,
		AppInterface * appinterface)
{
	size_t len;
	int i;
	char buf[8];
	VariableType type = VT_NULL;
	AppMessageCallDirection direction;
	char const * p;

	len = strlen(appinterface->prefix);
	if(key == NULL || strncmp(appinterface->prefix, key, len) != 0)
		return 0;
	key += len;
	if((p = hash_get(value, "ret")) != NULL && _string_enum(_string_type, p,
				&type, &direction) != 0)
	{
		appinterface->error = error_set_code(1, "%s",
				"Invalid return type");
		return -appinterface->error;
	}
	/* XXX ignoring the direction here */
	if(_new_append(appinterface, type, AMCD_OUT, key) != 0)
	{
		appinterface->error = 1;
		return -appinterface->error;
	}
	for(i = 0; i < APPINTERFACE_MAX_ARGUMENTS; i++)
	{
		snprintf(buf, sizeof(buf), "arg%d", i + 1);
		if((p = hash_get(value, buf)) == NULL)
			break;
		if(_new_append_arg(appinterface, p) != 0)
		{
			/* FIXME may crash here? */
			appinterface->error = 1;
			return -1;
		}
	}
	return 0;
}

static int _new_append(AppInterface * ai, VariableType type,
		AppMessageCallDirection direction, char const * call)
{
	AppInterfaceCall * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, \"%s\")\n", __func__, type, call);
#endif
	if((p = realloc(ai->calls, sizeof(*p) * (ai->calls_cnt + 1))) == NULL)
		return -1;
	ai->calls = p;
	p = &ai->calls[ai->calls_cnt];
	if((p->name = string_new(call)) == NULL)
		return -1;
	p->type.type = type;
	p->type.direction = direction;
	p->args = NULL;
	p->args_cnt = 0;
	ai->calls_cnt++;
	return 0;
}

static int _new_append_arg(AppInterface * ai, char const * arg)
{
	char buf[16];
	char * p;
	AppInterfaceCall * q;
	AppInterfaceCallArg * r;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, arg);
#endif
	snprintf(buf, sizeof(buf), "%s", arg);
	if((p = strchr(buf, ',')) != NULL)
		*p = '\0';
	q = &ai->calls[ai->calls_cnt - 1];
	if((r = realloc(q->args, sizeof(*r) * (q->args_cnt + 1))) == NULL)
		return -error_set_code(1, "%s", strerror(errno));
	q->args = r;
	r = &q->args[q->args_cnt];
	if(_string_enum(_string_type, buf, &r->type, &r->direction) != 0)
		return -1;
	q->args_cnt++;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() type %u, direction %u\n", __func__,
			r->type, r->direction);
#endif
	return 0;
}




/* accessors */
/* appinterface_get_call */
static AppInterfaceCall * _appinterface_get_call(AppInterface * appinterface,
		String const * call)
{
	size_t i;

	/* FIXME also check that the parameters match */
	for(i = 0; i < appinterface->calls_cnt; i++)
		if(string_compare(appinterface->calls[i].name, call) == 0)
			break;
	if(i == appinterface->calls_cnt)
	{
		error_set_code(1, "%s%s%s%s", "Unknown call ", call,
				" for interface ", appinterface->name);
		return NULL;
	}
	return &appinterface->calls[i];
}


/* string_enum */
static int _string_enum(StringEnum * se, String const * string,
		VariableType * type, AppMessageCallDirection * direction)
{
	size_t i;

	if(string == NULL)
		return -error_set_code(1, "%s", strerror(EINVAL));
	for(i = 0; se[i].string != NULL; i++)
		if(string_compare(string, se[i].string) == 0)
		{
			*type = se[i].type;
			*direction = se[i].direction;
			return 0;
		}
	return -error_set_code(1, "%s\"%s\"", "Unknown enumerated value for ",
			string);
}
