/* $Id$ */
/* Copyright (c) 2011-2022 Pierre Pronchery <khorben@defora.org> */
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include <System/Marshall.h>
#include "App/appmessage.h"
#include "App/appserver.h"
#include "appstatus.h"
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
#define VT_COUNT (VT_LAST + 1)
#define AICT_MASK 077

#ifdef DEBUG
static const String * AICTString[VT_COUNT] =
{
	"void", "bool", "int8", "uint8", "int16", "uint16", "int32", "uint32",
	"int64", "uint64", "float", "double", "Buffer", "String", "Array",
	"Compound"
};
#endif

typedef enum _AppInterfaceCallDirection
{
	AICD_IN		= 0100,
	AICD_OUT	= 0200,
	AICD_IN_OUT	= 0300,
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
	MarshallCall call;
} AppInterfaceCall;

struct _AppInterface
{
	AppTransportMode mode;
	String * name;
	Config * config;

	AppStatus * status;
	AppInterfaceCall * calls;
	size_t calls_cnt;
	AppInterfaceCall * callbacks;
	size_t callbacks_cnt;
	/* XXX for hash_foreach() in _new_interface_do() */
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
	/* implicit input */
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
	{ "BUFFER",	VT_BUFFER	| AICD_IN	},
	{ "FLOAT",	VT_FLOAT	| AICD_IN	},
	{ "DOUBLE",	VT_DOUBLE	| AICD_IN	},
	/* input aliases */
	{ "BOOL_IN",	VT_BOOL		| AICD_IN	},
	{ "INT8_IN",	VT_INT8		| AICD_IN	},
	{ "UINT8_IN",	VT_UINT8	| AICD_IN	},
	{ "INT16_IN",	VT_INT16	| AICD_IN	},
	{ "UINT16_IN",	VT_UINT16	| AICD_IN	},
	{ "INT32_IN",	VT_INT32	| AICD_IN	},
	{ "UINT32_IN",	VT_UINT32	| AICD_IN	},
	{ "INT64_IN",	VT_INT64	| AICD_IN	},
	{ "UINT64_IN",	VT_UINT64	| AICD_IN	},
	{ "STRING_IN",	VT_STRING	| AICD_IN	},
	{ "BUFFER_IN",	VT_BUFFER	| AICD_IN	},
	{ "FLOAT_IN",	VT_FLOAT	| AICD_IN	},
	{ "DOUBLE_IN",	VT_DOUBLE	| AICD_IN	},
	/* output only */
	{ "BOOL_OUT",	VT_BOOL		| AICD_OUT	},
	{ "INT8_OUT",	VT_INT8		| AICD_OUT	},
	{ "UINT8_OUT",	VT_UINT8	| AICD_OUT	},
	{ "INT16_OUT",	VT_INT16	| AICD_OUT	},
	{ "UINT16_OUT",	VT_UINT16	| AICD_OUT	},
	{ "INT32_OUT",	VT_INT32	| AICD_OUT	},
	{ "UINT32_OUT",	VT_UINT32	| AICD_OUT	},
	{ "INT64_OUT",	VT_INT64	| AICD_OUT	},
	{ "UINT64_OUT",	VT_UINT64	| AICD_OUT	},
	{ "STRING_OUT",	VT_STRING	| AICD_OUT	},
	{ "BUFFER_OUT",	VT_BUFFER	| AICD_OUT	},
	{ "FLOAT_OUT",	VT_FLOAT	| AICD_OUT	},
	{ "DOUBLE_OUT",	VT_DOUBLE	| AICD_OUT	},
	{ NULL,		0				}
};


/* prototypes */
static int _string_enum(String const * string, StringEnum const * se);

/* accessors */
static AppInterfaceCall * _appinterface_get_call(AppInterface * appinterface,
		char const * method);

/* useful */
static Variable ** _appinterface_argv_new(AppInterfaceCall * call,
		va_list ap);
static void _appinterface_argv_free(Variable ** argv, size_t argc);
static int _appinterface_call(App * app, AppServerClient * asc,
		Variable * result, AppInterfaceCall * call,
		size_t argc, Variable ** argv);
static AppMessage * _appinterface_message(AppInterfaceCall * call,
		size_t argc, Variable ** argv);


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
static String * _new_interface(String const * app);

AppInterface * appinterface_new(AppTransportMode mode, String const * app)
{
	AppInterface * ai;
	String * pathname;

	if((pathname = _new_interface(app)) == NULL)
		return NULL;
	ai = appinterface_new_interface(mode, app, pathname);
	string_delete(pathname);
	return ai;
}

static String * _new_interface(String const * app)
{
	String * var;
	String const * interface;

	if((var = string_new_append("APPINTERFACE_", app, NULL)) == NULL)
		return NULL;
	interface = getenv(var);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\" \"%s\"\n", __func__, var,
			interface);
#endif
	string_delete(var);
	if(interface != NULL)
		return string_new(interface);
	return string_new_append(SYSCONFDIR "/AppInterface/", app,
			".interface", NULL);
}


/* appinterface_new_interface */
static AppInterfaceCall * _new_interface_append_call(AppInterface * ai,
		VariableType type, char const * method);
static AppInterfaceCall * _new_interface_append_callback(AppInterface * ai,
		VariableType type, char const * method);
static int _new_interface_append_arg(AppInterfaceCall * call, char const * arg);
AppInterface * _new_interface_do(AppTransportMode mode, String const * app,
		String const * pathname);
static int _new_interface_do_appstatus(AppInterface * appinterface);
static int _new_interface_foreach_calls(char const * key, Hash * value,
		AppInterface * appinterface);
static int _new_interface_foreach_callbacks(char const * key, Hash * value,
		AppInterface * appinterface);
static AppInterface * _new_interface_mode_client(AppTransportMode mode,
		String const * app, String const * pathname);
static AppInterface * _new_interface_mode_server(AppTransportMode mode,
		String const * app, String const * pathname);

AppInterface * appinterface_new_interface(AppTransportMode mode,
		String const * app, String const * pathname)
{
	switch(mode)
	{
		case ATM_CLIENT:
			return _new_interface_mode_client(mode, app, pathname);
		case ATM_SERVER:
			return _new_interface_mode_server(mode, app, pathname);
	}
	return NULL;
}

static AppInterfaceCall * _new_interface_append_call(AppInterface * ai,
		VariableType type, char const * method)
{
	AppInterfaceCall * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, \"%s\")\n", __func__, type, method);
#endif
	if((p = realloc(ai->calls, sizeof(*p) * (ai->calls_cnt + 1))) == NULL)
		return NULL;
	ai->calls = p;
	p = &ai->calls[ai->calls_cnt];
	if((p->name = string_new(method)) == NULL)
		return NULL;
	p->type.type = type & AICT_MASK;
	p->type.direction = type & AICD_MASK;
	p->args = NULL;
	p->args_cnt = 0;
	ai->calls_cnt++;
	return p;
}

static AppInterfaceCall * _new_interface_append_callback(AppInterface * ai,
		VariableType type, char const * method)
{
	AppInterfaceCall * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, \"%s\")\n", __func__, type, method);
#endif
	if((p = realloc(ai->callbacks, sizeof(*p) * (ai->callbacks_cnt + 1)))
			== NULL)
		return NULL;
	ai->callbacks = p;
	p = &ai->callbacks[ai->callbacks_cnt];
	if((p->name = string_new(method)) == NULL)
		return NULL;
	p->type.type = type & AICT_MASK;
	p->type.direction = type & AICD_MASK;
	p->args = NULL;
	p->args_cnt = 0;
	ai->callbacks_cnt++;
	return p;
}

static int _new_interface_append_arg(AppInterfaceCall * call, char const * arg)
{
	char buf[16];
	char * p;
	int type;
	AppInterfaceCallArg * r;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, arg);
#endif
	snprintf(buf, sizeof(buf), "%s", arg);
	if((p = strchr(buf, ',')) != NULL)
		*p = '\0';
	if((type = _string_enum(buf, _string_type)) < 0)
		return -1;
	if((r = realloc(call->args, sizeof(*r) * (call->args_cnt + 1))) == NULL)
		return error_set_code(-errno, "%s", strerror(errno));
	call->args = r;
	r = &call->args[call->args_cnt++];
	r->type = type & AICT_MASK;
	r->direction = type & AICD_MASK;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: type %s, direction: %d\n", AICTString[r->type],
			r->direction);
#endif
	return 0;
}

AppInterface * _new_interface_do(AppTransportMode mode, String const * app,
		String const * pathname)
{
	AppInterface * appinterface;

	if(app == NULL)
		return NULL; /* FIXME report error */
	if((appinterface = object_new(sizeof(*appinterface))) == NULL)
		return NULL;
	appinterface->mode = mode;
	appinterface->name = string_new(app);
	appinterface->config = config_new();
	appinterface->status = NULL;
	appinterface->calls = NULL;
	appinterface->calls_cnt = 0;
	appinterface->callbacks = NULL;
	appinterface->callbacks_cnt = 0;
	appinterface->error = 0;
	if(appinterface->name == NULL
			|| appinterface->config == NULL
			|| config_load(appinterface->config, pathname) != 0
			|| _new_interface_do_appstatus(appinterface) != 0)
	{
		appinterface_delete(appinterface);
		return NULL;
	}
	appinterface->error = 0;
	hash_foreach(appinterface->config,
			(HashForeach)_new_interface_foreach_calls,
			appinterface);
	hash_foreach(appinterface->config,
			(HashForeach)_new_interface_foreach_callbacks,
			appinterface);
	if(appinterface->error != 0)
	{
		appinterface_delete(appinterface);
		return NULL;
	}
	return appinterface;
}

static int _new_interface_do_appstatus(AppInterface * appinterface)
{
	if((appinterface->status = appstatus_new_config(appinterface->config,
					"status")) == NULL)
		return -1;
	/* XXX delete and set status to NULL if empty */
	return 0;
}

static int _new_interface_foreach_callbacks(char const * key, Hash * value,
		AppInterface * appinterface)
{
	String const * prefix = APPINTERFACE_CALLBACK_PREFIX;
	unsigned int i;
	char buf[8];
	int type = VT_NULL;
	char const * p;
	AppInterfaceCall * callback;

	if(key == NULL || strncmp(prefix, key, string_get_length(prefix)) != 0)
		return 0;
	key += string_get_length(prefix);
	if((p = hash_get(value, "ret")) != NULL
			&& (type = _string_enum(p, _string_type)) < 0)
	{
		appinterface->error = error_set_code(1, "%s: %s", p,
				"Invalid return type for callback");
		return -appinterface->error;
	}
	if((callback = _new_interface_append_callback(appinterface, type, key))
			== NULL)
	{
		appinterface->error = 1;
		return -appinterface->error;
	}
	for(i = 0; i < APPSERVER_MAX_ARGUMENTS; i++)
	{
		snprintf(buf, sizeof(buf), "arg%u", i + 1);
		if((p = hash_get(value, buf)) == NULL)
			break;
		if(_new_interface_append_arg(callback, p) != 0)
		{
			/* FIXME may crash here? */
			appinterface->error = 1;
			return -1;
		}
	}
	return 0;
}

static int _new_interface_foreach_calls(char const * key, Hash * value,
		AppInterface * appinterface)
{
	String const * prefix = APPINTERFACE_CALL_PREFIX;
	unsigned int i;
	char buf[8];
	int type = VT_NULL;
	char const * p;
	AppInterfaceCall * call;

	if(key == NULL || strncmp(prefix, key, string_get_length(prefix)) != 0)
		return 0;
	key += string_get_length(prefix);
	if((p = hash_get(value, "ret")) != NULL
			&& (type = _string_enum(p, _string_type)) < 0)
	{
		appinterface->error = error_set_code(1, "%s: %s", p,
				"Invalid return type for call");
		return -appinterface->error;
	}
	if((call = _new_interface_append_call(appinterface, type, key)) == NULL)
	{
		appinterface->error = 1;
		return -appinterface->error;
	}
	for(i = 0; i < APPSERVER_MAX_ARGUMENTS; i++)
	{
		snprintf(buf, sizeof(buf), "arg%u", i + 1);
		if((p = hash_get(value, buf)) == NULL)
			break;
		if(_new_interface_append_arg(call, p) != 0)
		{
			/* FIXME may crash here? */
			appinterface->error = 1;
			return -1;
		}
	}
	return 0;
}

AppInterface * _new_interface_mode_client(AppTransportMode mode,
		String const * app, String const * pathname)
{
	AppInterface * ai;
	Plugin * plugin;
	size_t i;
	String * name;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, \"%s\", \"%s\")\n", __func__, mode, app,
			pathname);
#endif
	if((plugin = plugin_new_self()) == NULL)
		return NULL;
	if((ai = _new_interface_do(mode, app, pathname)) == NULL)
	{
		plugin_delete(plugin);
		return NULL;
	}
	for(i = 0; i < ai->callbacks_cnt; i++)
	{
		if((name = string_new_append(ai->name, "_",
						ai->callbacks[i].name, NULL))
				== NULL)
			break;
		ai->callbacks[i].call = plugin_lookup(plugin, name);
		string_delete(name);
		if(ai->callbacks[i].call == NULL)
			break;
	}
	plugin_delete(plugin);
	if(i != ai->callbacks_cnt)
	{
		appinterface_delete(ai);
		return NULL;
	}
	return ai;
}

AppInterface * _new_interface_mode_server(AppTransportMode mode,
		String const * app, String const * pathname)
{
	AppInterface * ai;
	Plugin * plugin;
	size_t i;
	String * name;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, \"%s\", \"%s\")\n", __func__, mode, app,
			pathname);
#endif
	if((plugin = plugin_new_self()) == NULL)
		return NULL;
	if((ai = _new_interface_do(mode, app, pathname)) == NULL)
	{
		plugin_delete(plugin);
		return NULL;
	}
	for(i = 0; i < ai->calls_cnt; i++)
	{
		if((name = string_new_append(ai->name, "_", ai->calls[i].name,
						NULL)) == NULL)
			break;
		ai->calls[i].call = plugin_lookup(plugin, name);
		string_delete(name);
		if(ai->calls[i].call == NULL)
			break;
	}
	plugin_delete(plugin);
	if(i != ai->calls_cnt)
	{
		appinterface_delete(ai);
		return NULL;
	}
	return ai;
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
	if(appinterface->status != NULL)
		appstatus_delete(appinterface->status);
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
		String const * method)
{
	AppInterfaceCall * aic;

	if((aic = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	*count = aic->args_cnt;
	return 0;
}


/* appinterface_get_status */
AppStatus * appinterface_get_status(AppInterface * appinterface)
{
	return appinterface->status;
}


/* useful */
/* appinterface_callv */
int appinterface_callv(AppInterface * appinterface, App * app,
		AppServerClient * asc, void ** result,
		String const * method, va_list args)
{
	int ret = 0;
	AppInterfaceCall * call;
	Variable * r;
	Variable ** argv;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	if(call->type.type == VT_NULL)
		r = NULL;
	else if((r = variable_new(call->type.type, NULL)) == NULL)
		return -1;
	if((argv = _appinterface_argv_new(call, args)) == NULL)
	{
		if(r != NULL)
			variable_delete(r);
		return -1;
	}
	if(ret == 0)
		ret = _appinterface_call(app, asc, r, call,
				call->args_cnt, argv);
	if(r != NULL)
	{
		if(ret == 0 && result != NULL)
			/* XXX return 0 anyway? */
			ret = variable_get_as(r, call->type.type, *result,
					NULL);
		variable_delete(r);
	}
	/* FIXME also implement AICD_{,IN}OUT */
	_appinterface_argv_free(argv, call->args_cnt);
	return ret;
}


/* appinterface_call_variablev */
int appinterface_call_variablev(AppInterface * appinterface, App * app,
		AppServerClient * asc, Variable * result, char const * method,
		size_t argc, Variable ** argv)
{
	AppInterfaceCall * call;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return -1;
	return _appinterface_call(app, asc, result, call, argc, argv);
}


/* appinterface_messagev */
AppMessage * appinterface_messagev(AppInterface * appinterface,
		char const * method, va_list ap)
{
	AppInterfaceCall * call;
	Variable ** argv;
	AppMessage * message;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return NULL;
	if((argv = _appinterface_argv_new(call, ap)) == NULL)
		return NULL;
	message = _appinterface_message(call, call->args_cnt, argv);
	_appinterface_argv_free(argv, call->args_cnt);
	return message;
}


/* appinterface_message_variables */
AppMessage * appinterface_message_variables(AppInterface * appinterface,
		char const * method, Variable ** args)
{
	AppInterfaceCall * call;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return NULL;
	return _appinterface_message(call, call->args_cnt, args);
}


/* appinterface_message_variablev */
AppMessage * appinterface_message_variablev(AppInterface * appinterface,
		char const * method, va_list ap)
{
	AppInterfaceCall * call;
	Variable ** argv;
	size_t i;
	AppMessage * message;

	if((call = _appinterface_get_call(appinterface, method)) == NULL)
		return NULL;
	if(call->args_cnt == 0)
		argv = NULL;
	else if((argv = malloc(sizeof(*argv) * call->args_cnt)) == NULL)
		/* XXX report error */
		return NULL;
	for(i = 0; i < call->args_cnt; i++)
		argv[i] = va_arg(ap, Variable *);
	message = _appinterface_message(call, call->args_cnt, argv);
	free(argv);
	return message;
}


/* private */
/* accessors */
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


/* useful */
/* appinterface_argv */
static Variable * _argv_new_in(VariableType type, va_list ap);
static Variable * _argv_new_in_out(VariableType type, va_list ap);
static Variable * _argv_new_out(VariableType type, va_list ap);

static Variable ** _appinterface_argv_new(AppInterfaceCall * call, va_list ap)
{
	Variable ** argv;
	size_t i;

	if((argv = object_new(sizeof(*argv) * (call->args_cnt))) == NULL)
		return NULL;
	for(i = 0; i < call->args_cnt; i++)
	{
		switch(call->args[i].direction)
		{
			case AICD_IN:
				argv[i] = _argv_new_in(call->args[i].type, ap);
				break;
			case AICD_IN_OUT:
				argv[i] = _argv_new_in_out(call->args[i].type,
						ap);
				break;
			case AICD_OUT:
				argv[i] = _argv_new_out(call->args[i].type,
						ap);
				break;
			default:
				/* XXX report the error */
				argv[i] = NULL;
				break;
		}
		if(argv[i] == NULL)
		{
			_appinterface_argv_free(argv, i);
			return NULL;
		}
	}
	return argv;
}

static Variable * _argv_new_in(VariableType type, va_list ap)
{
	return variable_newv(type, ap);
}

static Variable * _argv_new_in_out(VariableType type, va_list ap)
{
	union
	{
		bool * bp;
		int8_t * i8p;
		uint8_t * u8p;
		int16_t * i16p;
		uint16_t * u16p;
		int32_t * i32p;
		uint32_t * u32p;
		int64_t * i64p;
		uint64_t * u64p;
		char ** strp;
		Buffer * buf;
		float * fp;
		double * dp;
	} u;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, type);
#endif
	switch(type)
	{
		case VT_NULL:
			return variable_new(type);
		case VT_BOOL:
			u.bp = va_arg(ap, bool *);
			return variable_new(type, *u.bp);
		case VT_INT8:
			u.i8p = va_arg(ap, int8_t *);
			return variable_new(type, *u.i8p);
		case VT_UINT8:
			u.u8p = va_arg(ap, uint8_t *);
			return variable_new(type, *u.u8p);
		case VT_INT16:
			u.i16p = va_arg(ap, int16_t *);
			return variable_new(type, *u.i16p);
		case VT_UINT16:
			u.u16p = va_arg(ap, uint16_t *);
			return variable_new(type, *u.u16p);
		case VT_INT32:
			u.i32p = va_arg(ap, int32_t *);
			return variable_new(type, *u.i32p);
		case VT_UINT32:
			u.u32p = va_arg(ap, uint32_t *);
			return variable_new(type, *u.u32p);
		case VT_INT64:
			u.i64p = va_arg(ap, int64_t *);
			return variable_new(type, *u.i64p);
		case VT_UINT64:
			u.u64p = va_arg(ap, uint64_t *);
			return variable_new(type, *u.u64p);
		case VT_FLOAT:
			u.fp = va_arg(ap, float *);
			return variable_new(type, *u.fp);
		case VT_DOUBLE:
			u.dp = va_arg(ap, double *);
			return variable_new(type, *u.dp);
		case VT_STRING:
			u.strp = va_arg(ap, char **);
			return variable_new(type, *u.strp);
		case VT_BUFFER:
			u.buf = va_arg(ap, Buffer *);
			return variable_new(type, u.buf);
	}
	return NULL;
}

static Variable * _argv_new_out(VariableType type, va_list ap)
{
	union
	{
		bool * bp;
		int8_t * i8p;
		uint8_t * u8p;
		int16_t * i16p;
		uint16_t * u16p;
		int32_t * i32p;
		uint32_t * u32p;
		int64_t * i64p;
		uint64_t * u64p;
		char ** strp;
		Buffer * buf;
		float * fp;
		double * dp;
	} u;
	void * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, type);
#endif
	switch(type)
	{
		case VT_BOOL:
			u.bp = va_arg(ap, bool *);
			p = u.bp;
			break;
		case VT_INT8:
			u.i8p = va_arg(ap, int8_t *);
			p = u.i8p;
			break;
		case VT_UINT8:
			u.u8p = va_arg(ap, uint8_t *);
			p = u.u8p;
			break;
		case VT_INT16:
			u.i16p = va_arg(ap, int16_t *);
			p = u.i16p;
			break;
		case VT_UINT16:
			u.u16p = va_arg(ap, uint16_t *);
			p = u.u16p;
			break;
		case VT_INT32:
			u.i32p = va_arg(ap, int32_t *);
			p = u.i32p;
			break;
		case VT_UINT32:
			u.u32p = va_arg(ap, uint32_t *);
			p = u.u32p;
			break;
		case VT_INT64:
			u.i64p = va_arg(ap, int64_t *);
			p = u.i64p;
			break;
		case VT_UINT64:
			u.u64p = va_arg(ap, uint64_t *);
			p = u.u64p;
			break;
		case VT_STRING:
			u.strp = va_arg(ap, char **);
			p = u.strp;
			break;
		case VT_BUFFER:
			u.buf = va_arg(ap, Buffer *);
			p = u.buf;
			break;
		case VT_FLOAT:
			u.fp = va_arg(ap, float *);
			p = u.fp;
			break;
		case VT_DOUBLE:
			u.dp = va_arg(ap, double *);
			p = u.dp;
			break;
		case VT_NULL:
		default:
			p = NULL;
			break;
	}
	if(p == NULL)
		return NULL;
	return variable_new(type, NULL);
}


/* appinterface_argv_free */
static void _appinterface_argv_free(Variable ** argv, size_t argc)
{
	size_t i;

	for(i = 0; i < argc; i++)
		variable_delete(argv[i]);
	object_delete(argv);
}


/* appinterface_call */
static int _appinterface_call(App * app, AppServerClient * asc,
		Variable * result, AppInterfaceCall * call,
		size_t argc, Variable ** argv)
{
	int ret;
	Variable ** p;
	size_t i;

	if(argc != call->args_cnt)
		/* XXX set the error */
		return -1;
	if((p = object_new(sizeof(*p) * (argc + 2))) == NULL)
		return -1;
	p[0] = variable_new(VT_POINTER, app);
	p[1] = variable_new(VT_POINTER, asc);
	if(p[0] == NULL || p[1] == NULL)
	{
		if(p[0] != NULL)
			variable_delete(p[0]);
		if(p[1] != NULL)
			variable_delete(p[1]);
		object_delete(p);
		return -1;
	}
	for(i = 0; i < argc; i++)
		p[i + 2] = argv[i];
	ret = marshall_callp(result, call->call, argc + 2, p);
	variable_delete(p[1]);
	variable_delete(p[0]);
	object_delete(p);
	return ret;
}


/* appinterface_message */
static AppMessage * _appinterface_message(AppInterfaceCall * call,
		size_t argc, Variable ** argv)
{
	AppMessage * message;
	AppMessageCallArgument * args;
	size_t i;

	if(argc != call->args_cnt)
	{
		error_set_code(1, "%s: %s%zu%s%zu%s", call->name,
				"Invalid number of arguments (", argc,
				", expected: ", call->args_cnt, ")");
		return NULL;
	}
	if(argc == 0)
		args = NULL;
	else if((args = object_new(sizeof(*args) * argc)) == NULL)
		return NULL;
	else
		for(i = 0; i < argc; i++)
		{
			args[i].direction = call->args[i].direction;
			args[i].arg = argv[i];
		}
	message = appmessage_new_call(call->name, args, argc);
	object_delete(args);
	return message;
}
