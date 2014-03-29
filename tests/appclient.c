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
#include "App/appclient.h"


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: appclient [-a app][-n name]\n", stderr);
	return 1;
}


/* public */
/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * app = NULL;
	char const * name = NULL;
	AppClient * appclient;

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
	if((appclient = appclient_new(app, name)) == NULL)
	{
		error_print("appclient");
		return 2;
	}
	appclient_delete(appclient);
	return 0;
}
