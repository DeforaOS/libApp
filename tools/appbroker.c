/* $Id$ */
/* Copyright (c) 2011-2016 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App.h"

#define PROGNAME_APPBROKER "AppBroker"


/* AppBroker */
/* private */
/* types */
typedef struct _AppBrokerPrefs
{
	AppTransportMode mode;
	char const * outfile;
	int dryrun;
} AppBrokerPrefs;

typedef struct _AppBroker
{
	AppBrokerPrefs prefs;
	Config * config;
	char const * prefix;
	FILE * fp;
	int error;
} AppBroker;


/* prototypes */
static int _appbroker(AppBrokerPrefs * prefs, char const * filename);
static int _usage(void);


/* functions */
static void _appbroker_calls(AppBroker * appbroker);
static void _appbroker_callbacks(AppBroker * appbroker);
static void _appbroker_constants(AppBroker * appbroker);
static char const * _appbroker_ctype(char const * type);
static int _appbroker_do(AppBroker * appbroker, AppTransportMode mode);
static int _appbroker_foreach_call(char const * key, Hash * value, void * data);
static int _appbroker_foreach_call_arg(AppBroker * appbroker, char const * sep,
		char const * arg);
static int _appbroker_foreach_callback(char const * key, Hash * value,
		void * data);
static int _appbroker_foreach_constant(char const * key, char const * value,
		void * data);
static void _appbroker_head(AppBroker * appbroker);
static void _appbroker_tail(AppBroker * appbroker);

static int _appbroker(AppBrokerPrefs * prefs, char const * filename)
{
	AppBroker appbroker;

	if(prefs != NULL)
		appbroker.prefs = *prefs;
	else
	{
		memset(&appbroker.prefs, 0, sizeof(appbroker.prefs));
		appbroker.prefs.mode = ATM_SERVER;
	}
	if((appbroker.config = config_new()) == NULL)
		return error_print(PROGNAME_APPBROKER);
	if(config_load(appbroker.config, filename) != 0
			|| (appbroker.prefix = config_get(appbroker.config,
					NULL, "service")) == NULL)
	{
		config_delete(appbroker.config);
		return error_print(PROGNAME_APPBROKER);
	}
	appbroker.fp = NULL;
	if(_appbroker_do(&appbroker, appbroker.prefs.mode) == 0
			&& appbroker.prefs.dryrun == 0)
	{
		if(appbroker.prefs.outfile == NULL)
			appbroker.fp = stdout;
		else if((appbroker.fp = fopen(appbroker.prefs.outfile, "w"))
				== NULL)
		{
			config_delete(appbroker.config);
			return error_set_print(PROGNAME_APPBROKER, 1, "%s: %s",
					appbroker.prefs.outfile,
					strerror(errno));
		}
		_appbroker_do(&appbroker, appbroker.prefs.mode);
	}
	if(appbroker.fp != NULL && appbroker.prefs.outfile != NULL)
		fclose(appbroker.fp);
	config_delete(appbroker.config);
	return appbroker.error;
}

static void _appbroker_calls(AppBroker * appbroker)
{
	if(appbroker->fp != NULL)
		fputs("\n/* calls */\n", appbroker->fp);
	hash_foreach(appbroker->config,
			(HashForeach)_appbroker_foreach_call, appbroker);
}

static void _appbroker_callbacks(AppBroker * appbroker)
{
	if(appbroker->fp != NULL)
		fputs("\n\n/* callbacks */\n", appbroker->fp);
	hash_foreach(appbroker->config,
			(HashForeach)_appbroker_foreach_callback, appbroker);
}

static void _appbroker_constants(AppBroker * appbroker)
{
	Hash * hash;

	if((hash = hash_get(appbroker->config, "constants")) == NULL)
		return;
	if(appbroker->fp != NULL)
		fputs("\n\n/* constants */\n", appbroker->fp);
	hash_foreach(hash, (HashForeach)_appbroker_foreach_constant, appbroker);
}

static char const * _appbroker_ctype(char const * type)
{
	struct
	{
		char const * type;
		char const * ctype;
	} ctypes[] =
	{
		{ "VOID", "void", },
		{ "BOOL", "bool", },
		{ "INT8", "int8_t", },
		{ "UINT8", "uint8_t", },
		{ "INT16", "int16_t", },
		{ "UINT16", "uint16_t", },
		{ "INT32", "int32_t", },
		{ "UINT32", "uint32_t", },
		{ "INT64", "int64_t", },
		{ "UINT64", "uint64_t", },
		{ "FLOAT", "float", },
		{ "DOUBLE", "double", },
		{ "BUFFER", "Buffer const *", },
		{ "STRING", "String const *", },
		{ "BOOL_IN", "bool", },
		{ "INT8_IN", "int8_t", },
		{ "UINT8_IN", "uint8_t", },
		{ "INT16_IN", "int16_t", },
		{ "UINT16_IN", "uint16_t", },
		{ "INT32_IN", "int32_t", },
		{ "UINT32_IN", "uint32_t", },
		{ "INT64_IN", "int64_t", },
		{ "UINT64_IN", "uint64_t", },
		{ "FLOAT_IN", "float", },
		{ "DOUBLE_IN", "double", },
		{ "BUFFER_IN", "Buffer const *", },
		{ "STRING_IN", "String const *", },
		{ "BOOL_OUT", "bool *", },
		{ "INT8_OUT", "int8_t *", },
		{ "UINT8_OUT", "uint8_t *", },
		{ "INT16_OUT", "int16_t *", },
		{ "UINT16_OUT", "uint16_t *", },
		{ "INT32_OUT", "int32_t *", },
		{ "UINT32_OUT", "uint32_t *", },
		{ "INT64_OUT", "int64_t *", },
		{ "UINT64_OUT", "uint64_t *", },
		{ "FLOAT_OUT", "float *", },
		{ "DOUBLE_OUT", "double *", },
		{ "BUFFER_OUT", "Buffer *", },
		{ "STRING_OUT", "String **", },
		{ "BOOL_INOUT", "bool *", },
		{ "INT8_INOUT", "int8_t *", },
		{ "UINT8_INOUT", "uint8_t *", },
		{ "INT16_INOUT", "int16_t *", },
		{ "UINT16_INOUT", "uint16_t *", },
		{ "INT32_INOUT", "int32_t *", },
		{ "UINT32_INOUT", "uint32_t *", },
		{ "INT64_INOUT", "int64_t *", },
		{ "UINT64_INOUT", "uint64_t *", },
		{ "FLOAT_INOUT", "float *", },
		{ "DOUBLE_INOUT", "double *", },
		{ "BUFFER_INOUT", "Buffer *", },
		{ "STRING_INOUT", "String **" }
	};
	size_t i;

	for(i = 0; i < sizeof(ctypes) / sizeof(*ctypes); i++)
		if(strcmp(ctypes[i].type, type) == 0)
			return ctypes[i].ctype;
	return NULL;
}

static int _appbroker_do(AppBroker * appbroker, AppTransportMode mode)
{
	appbroker->error = 0;
	_appbroker_head(appbroker);
	_appbroker_constants(appbroker);
	(mode == ATM_SERVER) ? _appbroker_calls(appbroker)
		: _appbroker_callbacks(appbroker);
	_appbroker_tail(appbroker);
	return appbroker->error;
}

static int _appbroker_foreach_call(char const * key, Hash * value, void * data)
{
	AppBroker * appbroker = data;
	const char prefix[] = "call::";
	unsigned int i;
	char buf[8];
	char const * p;
	const char sep[] = ", ";

	if(key == NULL || key[0] == '\0')
		return 0;
	if(strncmp(key, prefix, sizeof(prefix) - 1) != 0)
		return 0;
	key += sizeof(prefix) - 1;
	if((p = hash_get(value, "ret")) == NULL)
		p = "VOID";
	if((p = _appbroker_ctype(p)) == NULL)
		appbroker->error = -error_set_print(PROGNAME_APPBROKER, 1,
				"%s: %s", key, "Invalid return type for call");
	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s%s%s%s%s%s", p, " ",
				appbroker->prefix, "_", key,
				"(App * app, AppServerClient * client");
	for(i = 0; i < APPSERVER_MAX_ARGUMENTS; i++)
	{
		snprintf(buf, sizeof(buf), "arg%u", i + 1);
		if((p = hash_get(value, buf)) == NULL)
			break;
		if(_appbroker_foreach_call_arg(appbroker, sep, p) != 0)
			return -1;
	}
	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s", ");\n");
	return 0;
}

static int _appbroker_foreach_call_arg(AppBroker * appbroker, char const * sep,
		char const * arg)
{
	char const * ctype;
	char * p;
	String * q;

	if((p = strchr(arg, ',')) == NULL)
		ctype = _appbroker_ctype(arg);
	else if((q = string_new_length(arg, p - arg)) != NULL)
	{
		ctype = _appbroker_ctype(q);
		string_delete(q);
	}
	else
	{
		appbroker->error = -1;
		return -1;
	}
	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s%s%s%s", sep, ctype,
				(p != NULL && string_length(p + 1)) ? " " : "",
				(p != NULL) ? p + 1 : "");
	return 0;
}

static int _appbroker_foreach_callback(char const * key, Hash * value,
		void * data)
{
	/* XXX some code duplication with _appbroker_foreach_call() */
	AppBroker * appbroker = data;
	const char prefix[] = "callback::";
	int i;
	char buf[8];
	char const * p;
	const char sep[] = ", ";

	if(key == NULL || key[0] == '\0')
		return 0;
	if(strncmp(key, prefix, sizeof(prefix) - 1) != 0)
		return 0;
	key += sizeof(prefix) - 1;
	if((p = hash_get(value, "ret")) == NULL)
		p = "VOID";
	if((p = _appbroker_ctype(p)) == NULL)
		appbroker->error = -error_set_print(PROGNAME_APPBROKER, 1,
				"%s: %s", key, "Unknown return type for"
				" callback");
	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s%s%s%s%s%s", p, " ",
				appbroker->prefix, "_", key,
				"(AppClient * client");
	for(i = 0; i < APPSERVER_MAX_ARGUMENTS; i++)
	{
		snprintf(buf, sizeof(buf), "arg%u", i + 1);
		if((p = hash_get(value, buf)) == NULL)
			break;
		if(_appbroker_foreach_call_arg(appbroker, sep, p) != 0)
			return -1;
	}
	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s", ");\n");
	return 0;
}

static int _appbroker_foreach_constant(char const * key, char const * value,
		void * data)
{
	AppBroker * appbroker = data;

	if(appbroker->fp != NULL)
		fprintf(appbroker->fp, "# define %s_%s\t%s\n",
				appbroker->prefix, key, value);
	return 0;
}

static void _appbroker_head(AppBroker * appbroker)
{
	if(appbroker->fp == NULL)
		return;
	fputs("/* $""Id$ */\n\n\n\n", appbroker->fp);
	if(appbroker->prefix != NULL)
		fprintf(appbroker->fp, "%s%s%s%s%s%s%s%s%s%s", "#ifndef ",
				appbroker->prefix, "_",
				appbroker->prefix, "_H\n", "# define ",
				appbroker->prefix, "_",
				appbroker->prefix, "_H\n");
	fputs("\n# include <stdbool.h>\n", appbroker->fp);
	fputs("# include <stdint.h>\n", appbroker->fp);
	fputs("# include <System/App.h>\n\n", appbroker->fp);
}

static void _appbroker_tail(AppBroker * appbroker)
{
	if(appbroker->prefix != NULL && appbroker->fp != NULL)
		fprintf(appbroker->fp, "%s%s%s%s%s", "\n#endif /* !",
				appbroker->prefix, "_",
				appbroker->prefix, "_H */\n");
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_APPBROKER " [-cns][-o outfile] filename\n"
"  -n	Only check for errors (dry-run)\n", stderr);
	return 1;
}


/* main */
int main(int argc, char * argv[])
{
	int o;
	AppBrokerPrefs prefs;

	memset(&prefs, 0, sizeof(prefs));
	prefs.mode = ATM_SERVER;
	while((o = getopt(argc, argv, "cno:s")) != -1)
		switch(o)
		{
			case 'c':
				prefs.mode = ATM_CLIENT;
				break;
			case 'n':
				prefs.dryrun = 1;
				break;
			case 'o':
				prefs.outfile = optarg;
				break;
			case 's':
				prefs.mode = ATM_SERVER;
				break;
			default:
				return _usage();
		}
	if(optind + 1 != argc)
		return _usage();
	return (_appbroker(&prefs, argv[optind]) == 0) ? 0 : 2;
}
