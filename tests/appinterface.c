/* $Id$ */
/* Copyright (c) 2017 Pierre Pronchery <khorben@defora.org> */
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
#include "../src/appinterface.h"


/* private */
/* functions */
/* usage */
static int _usage(void)
{
	fputs("Usage: appinterface -a app\n", stderr);
	return 1;
}


/* public */
/* main */
int main(int argc, char * argv[])
{
	int o;
	String const * app = NULL;
	String * path;
	AppInterface * appinterface;

	while((o = getopt(argc, argv, "a:")) != -1)
		switch(o)
		{
			case 'a':
				app = optarg;
				break;
			default:
				return _usage();
		}
	if(optind != argc || app == NULL)
		return _usage();
	if((path = string_new_append("../data/", app, ".interface", NULL))
			== NULL)
	{
		error_print("appinterface");
		return 2;
	}
	appinterface = appinterface_new_interface(ATM_SERVER, app, path);
	string_delete(path);
	if(appinterface == NULL)
	{
		error_print("appinterface");
		return 3;
	}
	appinterface_delete(appinterface);
	return 0;
}
