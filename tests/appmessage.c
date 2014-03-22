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



#include <stddef.h>
#include <System.h>
#include "App/appmessage.h"


/* public */
/* main */
int main(int argc, char * argv[])
{
	int ret = 0;
	AppMessage * message;
	Buffer * buffer;
	char const * call;

	if((message = appmessage_new_call("test", NULL, 0)) == NULL)
		return 2;
	if((buffer = buffer_new(0, NULL)) == NULL)
	{
		appmessage_delete(message);
		return 3;
	}
	if(appmessage_serialize(message, buffer) != 0)
		ret = 4;
	else
	{
		appmessage_delete(message);
		if((message = appmessage_new_deserialize(buffer)) == NULL)
			ret = 5;
		else if((call = appmessage_get_method(message)) == NULL)
			ret = 6;
		else if(strcmp(call, "test") != 0)
			ret = 7;
	}
	buffer_delete(buffer);
	if(message != NULL)
		appmessage_delete(message);
	return ret;
}
