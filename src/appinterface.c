/* $Id$ */
/* Copyright (c) 2011-2015 Pierre Pronchery <khorben@defora.org> */
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
/* TODO:
 * - implement what's missing */



#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App/appserver.h"
#include "marshall.h"
#include "appinterface.h"
#include "../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef SYSCONFDIR
# define SYSCONFDIR	PREFIX "/etc"
#endif


/* AppInterface */
/* private */
/* types */
/* XXX get rid of this */
#define VT_LAST VT_STRING
#define VT_COUNT (VT_LAST + 1)
#define AICT_MASK 077

#ifdef DEBUG
static const String * AICTString[VT_COUNT] =
{
	"void", "bool", "int8", "uint8", "int16", "uint16", "int32", "uint32",
	"int64", "uint64", "String", "Buffer", "float", "double"
};
#endif

typedef enum _AppInterfaceCallDirection
{
	AICD_IN		= 0000,
	AICD_IN_OUT	= 0100,
	AICD_OUT	= 0200
} AppInterfaceCallDirection;
#define AICD_MASK 0700

typedef struct _AppInterfaceCallArg
{
	VariableType type;
	AppInterfaceCallDirection direction;
	size_t size;
} AppInterfaceCallArg;

typedef struct _AppInterfaceCall
{
	char * name;
	AppInterfaceCallArg type;
	AppInterfaceCallArg * args;
	size_t args_cnt;
	void * (*func)(void *);
} AppInterfaceCall;

struct _AppInterface
{
	AppTransportMode mode;
	String * name;
	Config * config;

	AppInterfaceCall * calls;
	size_t calls_cnt;
	int error;
};

typedef struct _StringEnum
{
	char const * string;
	int value;
} StringEnum;


/* constants */
#define APPINTERFACE_CALL_PREFIX	"call::"
#define APPINTERFACE_CALLBACK_PREFIX	"callback::"


/* variables */
static const StringEnum _string_type[] =
{
	{ "VOID",	VT_NULL		| AICD_IN	},
	{ "BOOL",	VT_BOOL		| AICD_IN	},
	{ "INT8",	VT_INT8		| AICD_IN	},
	{ "UINT8",	VT_UINT8	| AICD_IN	},
	{ "INT16",	VT_INT16	| AICD_IN	},
	{ "UINT16",	VT_UINT16	| AICD_IN	},
	{ "INT32",	VT_INT32	| AICD_IN	},
	{ "UINT32",	VT_UINT32	| AICD_IN	},
	{ "INT64",	VT_INT64	| AICD_IN	},
	{ "UINT64",	VT_UINT64	| AICD_IN	},
	{ "STRING",	VT_STRING	| AICD_IN	},
	{ "STRING_OUT",	VT_STRING	| AICD_OUT	},
	{ "BUFFER",	VT_BUFFER	| AICD_IN	},
	{ "BUFFER_OUT",	VT_BUFFER	| AICD_OUT	},
	{ "FLOAT",	VT_FLOAT	| AICD_IN	},
	{ "DOUBLE",	VT_DOUBLE	| AICD_IN	},
	{ NULL,		0				}
};


/* prototypes */
static int _string_enum(String const * string, StringEnum const * se);

/* accessors */
static AppInterfaceCall * _appinterface_get_call(AppInterface * appinterface,
		char const * method);


/* functions */
/* string_enum */
/* FIXME move to string.c */
static int _string_enum(String const * string, StringEnum const * se)
{
	size_t i;

	if(string == NULL)
		return -error_set_code(-EINVAL, "%s", strerror(EINVAL));
	for(i = 0; se[i].string != NULL; i++)
		if(string_compare(string, se[i].string) == 0)
			return se[i].value;
	return -error_set_code(1, "%s\"%s\"", "Unknown enumerated value for ",
			string);
}


/* public */
/* functions */
/* appinterface_new */
static int _new_append(AppInterface * ai, VariableType type,
		char const * function);
static int _new_append_arg(AppInterface * ai, char const * arg);
AppInterface * _new_do(AppTransportMode mode, char const * app);
static int _new_foreach(char const * key, Hash * value,
		AppInterface * appinterface);

AppInterface * appinterface_new(AppTransportMode mode, char const * app)
{
	AppInterface * ai;
	Plugin * handle;
	size_t i;
	String * name;

	if((handle = plugin_new_self()) == NULL)
		return NULL;
	if((ai = _new_do(mode, app)) == NULL)
		return NULL;
	for(i = 0; i < ai->calls_cnt; i++)
	{
		if((name = string_new_append(ai->name, "_", ai->calls[i].name,
						NULL)) == NULL)
		{
			appinterface_delete(ai);
			ai = NULL;
			break;
		}
		ai->calls[i].func = plugin_lookup(handle, name);
		string_delete(name);
		if(ai->calls[i].func == NULL)
		{
			appinterface_delete(ai);
			ai = NULL;
			break;
		}
	}
	plugin_delete(handle);
	return ai;
}

static int _new_append(AppInterface * ai, VariableType type,
		char const * function)
{
	AppInterfaceCall * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, \"%s\")\n", __func__, type, function);
#endif
	if((p = realloc(ai->calls, sizeof(*p) * (ai->calls_cnt + 1))) == NULL)
		return -1;
	ai->calls = p;
	p = &ai->calls[ai->calls_cnt];
	if((p->name = string_new(function)) == NULL)
		return -1;
	p->type.type = type & AICT_MASK;
	p->type.direction = type & AICD_MASK;
	p->args = NULL;
	p->args_cnt = 0;
	ai->calls_cnt++;
	return 0;
}

static int _new_append_arg(AppInterface * ai, char const * arg)
{
	char buf[16];
	char * p;
	int type;
	AppInterfaceCall * q;
	AppInterfaceCallArg * r;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, arg);
#endif
	snprintf(buf, sizeof(buf), "%s", arg);
	if((p = strchr(buf, ',')) != NULL)
		*p = '\0';
	if((type = _string_enum(buf, _string_type)) < 0)
		return -1;
	q = &ai->calls[ai->calls_cnt - 1];
	if((r = realloc(q->args, sizeof(*r) * (q->args_cnt + 1))) == NULL)
		return error_set_code(-errno, "%s", strerror(errno));
	q->args = r;
	r = &q->args[q->args_cnt++];
	r->type = type & AICT_MASK;
	r->direction = type & AICD_MASK;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: type %s, direction: %d\n", AICTString[r->type],
			r->direction);
#endif
	return 0;
}

AppInterface * _new_do(AppTransportMode mode, char const * app)
{
	AppInterface * appinterface;
	String * pathname = NULL;

	if(app == NULL)
		return NULL; /* FIXME report error */
	if((appinterface = object_new(sizeof(*appinterface))) == NULL)
		return NULL;
	appinterface->mode = mode;
	appinterface->name = string_new(app);
	appinterface->config = config_new();
	appinterface->calls = NULL;
	appinterface->calls_cnt = 0;
	appinterface->error = 0;
	if(appinterface->name == NULL
			|| (pathname = string_new_append(
					SYSCONFDIR "/AppInterface/", app,
					".interface", NULL)) == NULL
			|| appinterface->config == NULL
			|| config_load(appinterface->config, pathname) != 0)
	{
		string_delete(pathname);
		appinterface_delete(appinterface);
		return NULL;
	}
	appinterface->error = 0;
	hash_foreach(appinterface->config, (HashForeach)_new_foreach,
			appinterface);
	if(appinterface->error != 0)
	{
		appinterface_delete(appinterface);
		return NULL;
	}
	return appinterface;
}

static int _new_foreach(char const * key, Hash * value,
		AppInterface * appinterface)
{
	String const * prefix = (appinterface->mode == ATM_SERVER)
		? APPINTERFACE_CALL_PREFIX : APPINTERFACE_CALLBACK_PREFIX;
	int i;
	char buf[8];
	int type = VT_NULL;
	char const * p;

	if(key == NULL || strncmp(prefix, key, string_length(prefix)) != 0)
		return 0;
	key += string_length(prefix);
	if((p = hash_get(value, "ret")) != NULL
			&& (type = _string_enum(p, _string_type)) < 0)
	{
		appinterface->error = error_set_code(1, "%s",
				"Invalid return type");
		return -appinterface->error;
	}
	if(_new_append(appinterface, type, key) != 0)
	{
		appinterface->error = 1;
		return -appinterface->error;
	}
	for(i = 0; i < APPSERVER_MAX_ARGUMENTS; i++)
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


/* appinterface_delete */
void appinterface_delete(AppInterface * appinterface)
{
	size_t i;

	if(appinterface->config != NULL)
		config_delete(appinterface->config);
	for(i = 0; i < appinterface->calls_cnt; i++)
	{
		string_delete(appinterface->calls[i].name);
		free(appinterface->calls[i].args);
	}
	free(appinterface->calls);
	string_delete(appinterface->name);
	object_delete(appinterface);
}


/* accessors */
/* appinterface_can_call */
static int _can_call_client(AppInterface * appinterface,
		AppInterfaceCall * call, char const * name);
static int _can_call_server(AppInterface * appinterface,
		AppInterfaceCall * call, char const * name);

int appinterface_can_call(AppInterface * appinterface, char const * method,
		char const * name)
{
	AppInterfaceCall * call;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	switch(appinterface->mode)
	{
		case ATM_CLIENT:
			return _can_call_client(appinterface, call, name);
		case ATM_SERVER:
			return _can_call_server(appinterface, call, name);
	}
	return -error_set_code(1, "%s", "Unknown AppInterface mode");
}

static int _can_call_client(AppInterface * appinterface,
		AppInterfaceCall * call, char const * name)
{
	/* FIXME really implement */
	return 1;
}

static int _can_call_server(AppInterface * appinterface,
		AppInterfaceCall * call, char const * name)
{
	int ret = 0;
	String * section;
	String const * allow;
	String const * deny;

	if((section = string_new_append(APPINTERFACE_CALL_PREFIX, call->name,
					NULL)) == NULL)
		return -1;
	allow = config_get(appinterface->config, section, "allow");
	deny = config_get(appinterface->config, section, "deny");
	/* FIXME implement pattern matching */
	if(allow == NULL && deny == NULL)
		ret = 1;
	else if(deny != NULL)
		ret = (strcmp(deny, name) == 0) ? 0 : 1;
	else
		ret = (strcmp(allow, name) == 0) ? 1 : 0;
	string_delete(section);
	return ret;
}


/* appinterface_get_name */
char const * appinterface_get_app(AppInterface * appinterface)
{
	return appinterface->name;
}


/* appinterface_get_args_count */
int appinterface_get_args_count(AppInterface * appinterface, size_t * count,
		char const * function)
{
	AppInterfaceCall * aic;

	if((aic = _appinterface_get_call(appinterface, function)) == NULL)
		return -1;
	*count = aic->args_cnt;
	return 0;
}


/* useful */
/* appinterface_callv */
int appinterface_callv(AppInterface * appinterface, App * app, void ** result,
		char const * method, va_list args)
{
	int ret;
	AppInterfaceCall * call;
	Variable * r;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	/* FIXME implement argv */
	if(call->type.type == VT_NULL)
		r = NULL;
	else if((r = variable_new(call->type.type, NULL)) == NULL)
		return -1;
	if((ret = marshall_call(app, call->type.type, r, call->func,
					call->args_cnt, NULL)) == 0
			&& r != NULL)
	{
		if(result != NULL)
			/* XXX return 0 anyway? */
			ret = variable_get_as(r, call->type.type, *result);
		variable_delete(r);
	}
	return ret;
}


/* appinterface_call_variablev */
int appinterface_call_variablev(AppInterface * appinterface, App * app,
		Variable * result, char const * method,
		size_t argc, Variable ** argv)
{
	AppInterfaceCall * call;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	if(argc != call->args_cnt)
		return -1;
	/* FIXME implement argv */
	return marshall_call(app, call->type.type, result, call->func, argc,
			NULL);
}


/* private */
/* appinterface_get_call */
static AppInterfaceCall * _appinterface_get_call(AppInterface * appinterface,
		String const * method)
{
	size_t i;

	for(i = 0; i < appinterface->calls_cnt; i++)
		if(string_compare(appinterface->calls[i].name, method) == 0)
			return &appinterface->calls[i];
	error_set_code(1, "%s%s%s%s", "Unknown call \"", method,
			"\" for interface ", appinterface->name);
	return NULL;
}
