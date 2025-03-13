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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <System.h>
#include "App.h"

#ifndef PROGNAME
# define PROGNAME_APPCLIENT "AppClient"
#endif


/* AppClient */
/* private */
/* types */
typedef enum _AppClientCallArgType
{
	ACCAT_DOUBLE = 0, ACCAT_FLOAT, ACCAT_INTEGER, ACCAT_STRING
} AppClientCallArgType;

typedef struct _AppClientCallArg
{
	AppClientCallArgType type;
	double _double;
	float _float;
	int integer;
	char const * string;
} AppClientCallArg;

typedef struct _AppClientCall
{
	char const * name;
	AppClientCallArg * args;
	size_t args_cnt;
} AppClientCall;


/* prototypes */
static int _appclient(int verbose, char const * app, char const * name,
		AppClientCall calls[], size_t calls_cnt);

static int _error(char const * message, int ret);


/* functions */
static int _appclient_call(int verbose, AppClient * ac, AppClientCall * call);

static int _appclient(int verbose, char const * app, char const * name,
		AppClientCall calls[], size_t calls_cnt)
{
	int ret = 0;
	AppClient * ac;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%d, %s, %s, %p, %zu)\n", __func__, verbose,
			app, name, (void *)calls, calls_cnt);
#endif
	if((ac = appclient_new(NULL, app, name)) == NULL)
		return _error(PROGNAME_APPCLIENT, 1);
	if(verbose != 0)
		puts("Connected.");
	for(i = 0; i < calls_cnt; i++)
		if(_appclient_call(verbose, ac, &calls[i]) != 0)
		{
			ret |= _error(PROGNAME_APPCLIENT, 1);
			break;
		}
	if(verbose != 0)
		puts("Disconnecting");
	appclient_delete(ac);
	return ret;
}

static int _appclient_call(int verbose, AppClient * ac, AppClientCall * call)
{
	int ret = 0;
	Variable * v;
	VariableType type;
	int64_t res;
	double dres;
	String * sres;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(verbose != 0)
		printf("Calling %s() with %lu arguments\n", call->name,
				(unsigned long)call->args_cnt);
	if((v = variable_new(VT_NULL, NULL)) == NULL)
		return -1;
	/* FIXME may segfault (check interface), use appclient_callv? */
	switch(call->args_cnt)
	{
		case 0:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %s() res=%ld\n", __func__,
					call->name, res);
#endif
			ret = appclient_call_variable(ac, v, call->name, NULL);
			break;
		case 1:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %s(%d)\n", __func__,
					call->name, call->args[0].integer);
#endif
			if(call->args[0].type == ACCAT_DOUBLE)
				ret = appclient_call_variable(ac, v, call->name,
						call->args[0]._double);
			else if(call->args[0].type == ACCAT_FLOAT)
				ret = appclient_call_variable(ac, v, call->name,
						call->args[0]._float);
			else if(call->args[0].type == ACCAT_INTEGER)
				ret = appclient_call_variable(ac, v, call->name,
						call->args[0].integer);
			else if(call->args[0].type == ACCAT_STRING)
				ret = appclient_call_variable(ac, v, call->name,
						call->args[0].string);
			else
				ret = error_set_code(1, "%s",
						"Unsupported types");
			break;
		case 2:
			/* FIXME arguments may be of different types */
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %s(%d, %d)\n", __func__,
					call->name,
					call->args[0].integer,
					call->args[1].integer);
#endif
			ret = appclient_call_variable(ac, v, call->name,
					call->args[0].integer,
					call->args[1].integer);
			break;
		case 3:
			if(strcmp(call->name, "glTranslatef") == 0)
			{
#ifdef DEBUG
				fprintf(stderr, "DEBUG: %s() %s(%.1f, %.1f,"
						" %.1f)\n", __func__,
						call->name,
						call->args[0]._float,
						call->args[1]._float,
						call->args[2]._float);
#endif
				ret = appclient_call_variable(ac, v, call->name,
						call->args[0]._float,
						call->args[1]._float,
						call->args[2]._float);
				break;
			}
			/* FIXME arguments may be of different types */
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %s(%d, %d, %d)\n",
					__func__, call->name,
					call->args[0].integer,
					call->args[1].integer,
					call->args[2].integer);
#endif
			ret = appclient_call_variable(ac, v, call->name,
					call->args[0].integer,
					call->args[1].integer,
					call->args[2].integer);
			break;
		case 4:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() %s(%.1f, %.1f, %.1f,"
					" %.1f)\n", __func__, call->name,
					call->args[0]._float,
					call->args[1]._float,
					call->args[2]._float,
					call->args[3]._float);
#endif
			ret = appclient_call_variable(ac, v, call->name,
					call->args[0]._float,
					call->args[1]._float,
					call->args[2]._float,
					call->args[3]._float);
			break;
		default:
			return error_set_code(1, "%s",
					"Unsupported number of arguments");
	}
	if(ret == 0 && verbose)
	{
		type = (v != NULL) ? variable_get_type(v) : VT_NULL;
		switch(type)
		{
			case VT_BOOL:
			case VT_INT8:
			case VT_UINT8:
			case VT_INT16:
			case VT_UINT16:
			case VT_INT32:
			case VT_UINT32:
			case VT_INT64:
			case VT_UINT64:
				if(variable_get_as(v, VT_INT64, &res, NULL)
						== 0)
					printf("\"%s\"%s%ld\n", call->name,
							" returned ", res);
				else
					printf("\"%s\"%s\n", call->name,
							" returned");
				break;
			case VT_FLOAT:
			case VT_DOUBLE:
				if(variable_get_as(v, VT_DOUBLE, &dres, NULL)
						== 0)
					printf("\"%s\"%s%f\n", call->name,
							" returned ", dres);
				else
					printf("\"%s\"%s\n", call->name,
							" returned");
				break;
			case VT_STRING:
				sres = NULL;
				if(variable_get_as(v, VT_STRING, &sres, NULL)
						== 0)
					printf("\"%s\"%s\"%s\"\n", call->name,
							" returned ", sres);
				else
					printf("\"%s\"%s\n", call->name,
							" returned");
				string_delete(sres);
				break;
			default:
				printf("\"%s\"%s\n", call->name, " returned");
				break;
		}
	}
	if(v != NULL)
		variable_delete(v);
	return ret;
}


/* error */
static int _error(char const * message, int ret)
{
	error_print(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME_APPCLIENT " [-v][-H hostname] -S service"
" [-C call [-d double|-f float|-i integer|-s string]...]...\n"
"  -v	Be more verbose\n"
"  -H	Hostname to connect to\n"
"  -S	Service to connect to\n"
"  -C	Enqueue a given call\n"
"  -d	Add a double as an argument to the current call\n"
"  -f	Add a float as an argument to the current call\n"
"  -i	Add an integer as an argument to the current call\n"
"  -s	Add a string as an argument to the current call\n", stderr);
	return 1;
}


/* main */
static int _main_call(AppClientCall ** calls, size_t * calls_cnt,
		char const * name);
static int _main_call_arg(AppClientCall * calls, size_t calls_cnt,
		AppClientCallArgType type, char const * string);

int main(int argc, char * argv[])
{
	int o;
	int res;
	int verbose = 0;
	char const * app = NULL;
	char const * name = NULL;
	AppClientCall * calls = NULL;
	size_t calls_cnt = 0;

	while((o = getopt(argc, argv, "vH:S:C:d:f:i:s:")) != -1)
	{
		res = 0;
		switch(o)
		{
			case 'v':
				verbose = 1;
				break;
			case 'H':
				name = optarg;
				break;
			case 'S':
				app = optarg;
				break;
			case 'C':
				res = _main_call(&calls, &calls_cnt, optarg);
				break;
			case 'd':
				res = _main_call_arg(calls, calls_cnt,
						ACCAT_DOUBLE, optarg);
				break;
			case 'f':
				res = _main_call_arg(calls, calls_cnt,
						ACCAT_FLOAT, optarg);
				break;
			case 'i':
				res = _main_call_arg(calls, calls_cnt,
						ACCAT_INTEGER, optarg);
				break;
			case 's':
				res = _main_call_arg(calls, calls_cnt,
						ACCAT_STRING, optarg);
				break;
			default:
				return _usage();
		}
		if(res != 0)
			return _error(PROGNAME_APPCLIENT, 2);
	}
	if(app == NULL)
		return _usage();
	return (_appclient(verbose, app, name, calls, calls_cnt) == 0) ? 0 : 2;
}

static int _main_call(AppClientCall ** calls, size_t * calls_cnt,
		char const * name)
{
	AppClientCall * p;

	if((p = realloc(*calls, sizeof(*p) * ((*calls_cnt) + 1))) == NULL)
		return error_set_code(1, "%s", strerror(errno));
	*calls = p;
	p = &(*calls)[(*calls_cnt)++];
	memset(p, 0, sizeof(*p));
	p->name = name;
	p->args = NULL;
	p->args_cnt = 0;
	return 0;
}

static int _main_call_arg(AppClientCall * calls, size_t calls_cnt,
		AppClientCallArgType type, char const * string)
{
	AppClientCall * p;
	AppClientCallArg * q;

	if(calls_cnt == 0)
		return 1; /* FIXME report error */
	p = &calls[calls_cnt - 1];
	if((q = realloc(p->args, sizeof(*q) * (p->args_cnt + 1))) == NULL)
		return error_set_code(1, "%s", strerror(errno));
	p->args = q;
	q = &q[p->args_cnt++];
	memset(q, 0, sizeof(*q));
	q->type = type;
	switch(type)
	{
		case ACCAT_DOUBLE:
			q->_double = strtod(string, NULL); /* XXX check */
			break;
		case ACCAT_FLOAT:
			q->_float = strtof(string, NULL); /* XXX check */
			break;
		case ACCAT_INTEGER:
			q->integer = strtol(string, NULL, 0); /* XXX check */
			break;
		case ACCAT_STRING:
			q->string = string;
			break;
	}
	return 0;
}
