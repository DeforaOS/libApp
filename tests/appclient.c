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



#include <System/error.h>
#include "App/appclient.h"


/* public */
/* main */
int main(int argc, char * argv[])
{
	AppClient * appclient;

	if((appclient = appclient_new("VFS", "tcp")) == NULL)
	{
		error_print("appclient");
		return 2;
	}
	appclient_delete(appclient);
	return 0;
}
