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
#include "App/appserver.h"
/* FIXME does not survive OBJDIR */
#include "Dummy.h"


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: appserver [-a app][-n name]\n", stderr);
	return 1;
}


/* public */
/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * app = NULL;
	char const * name = NULL;
	AppServer * appserver;

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
	if((appserver = appserver_new(NULL, 0, app, name)) == NULL)
	{
		error_print("appserver");
		return 2;
	}
	appserver_delete(appserver);
	return 0;
}
