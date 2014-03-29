/* $Id$ */
/* Copyright (c) 2014 Pierre Pronchery <khorben@defora.org> */
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
#include <System/error.h>
#include <System/string.h>
#include "../src/apptransport.h"


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: lookup [-a app][-n name]\n", stderr);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * app = NULL;
	char const * name = NULL;
	Event * event;
	AppTransport * transport;

	while((o = getopt(argc, argv, "a:n:")) != -1)
		switch(o)
		{
			case 'a':
				app = optarg;
				break;
			case 'n':
				name = optarg;
				break;
			default:
				return _usage();
		}
	if((event = event_new()) == NULL)
		return error_print("lookup");
	if((transport = apptransport_new_app(ATM_CLIENT, NULL, app, name,
					event)) == NULL)
	{
		event_delete(event);
		return error_print("lookup");
	}
	printf("transport: %s, name: %s\n",
			apptransport_get_transport(transport),
			apptransport_get_name(transport));
	apptransport_delete(transport);
	return 0;
}
