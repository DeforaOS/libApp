/* $Id$ */
/* Copyright (c) 2025 Pierre Pronchery <khorben@defora.org> */
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
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <System/error.h>
#include <System/plugin.h>
#include "App/apptransport.h"
#include "../config.h"

#ifndef PROGNAME_APPTRANSPORT
# define PROGNAME_APPTRANSPORT "AppTransport"
#endif


/* AppTransport */
/* private */
/* prototypes */
static int _apptransport_list(void);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* apptransport_list */
static int _apptransport_list(void)
{
#if defined(__APPLE__)
	char const soext[] = ".dylib";
#elif defined(__WIN32__)
	char const soext[] = ".dll";
#else
	char const soext[] = ".so";
#endif
	DIR * dir;
	struct dirent * de;
	size_t len;
	String * path;
	Plugin * plugin;
	AppTransportPluginDefinition * transport;

	if((dir = opendir(APP_TRANSPORT_PATH)) == NULL)
		return _error(APP_TRANSPORT_PATH, -1);
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(soext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(soext) + 1], soext) != 0)
			continue;
		if((path = string_new_append(APP_TRANSPORT_PATH "/", de->d_name,
						NULL)) == NULL)
		{
			_error(PROGNAME_APPTRANSPORT, -1);
			continue;
		}
		plugin = plugin_new_path(path);
		string_delete(path);
		if(plugin == NULL)
		{
			_error(PROGNAME_APPTRANSPORT, -1);
			continue;
		}
		if((transport = plugin_lookup(plugin, "transport")) == NULL)
		{
			_error(PROGNAME_APPTRANSPORT, -1);
			plugin_delete(plugin);
			continue;
		}
		de->d_name[len - sizeof(soext) + 1] = '\0';
		printf("%s (%s): %s\n", de->d_name, transport->name,
				transport->description);
		plugin_delete(plugin);
	}
	closedir(dir);
	return 0;
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
	fputs("Usage: " PROGNAME_APPTRANSPORT " -l\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int list = 0;

	while((o = getopt(argc, argv, "l")) != -1)
		switch(o)
		{
			case 'l':
				list = 1;
				break;
			default:
				return _usage();
		}
	if(optind != argc || list == 0)
		return _usage();
	return (_apptransport_list() == 0) ? 0 : 2;
}
