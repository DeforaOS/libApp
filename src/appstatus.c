/* $Id$ */
/* Copyright (c) 2015-2022 Pierre Pronchery <khorben@defora.org> */
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



#include <string.h>
#include <System.h>
#include "appstatus.h"


/* AppStatus */
/* private */
/* types */
struct _AppStatus
{
	Variable * variable;
};


/* public */
/* methods */
/* appstatus_new */
static void _new_config_foreach_section(String const * variable,
		String const * value, void * data);

AppStatus * appstatus_new_config(Config * config, String const * section)
{
	AppStatus * appstatus;
	struct
	{
		size_t members;
		String const ** names;
		Variable ** variables;
	} data;

	if((appstatus = object_new(sizeof(*appstatus))) == NULL)
		return NULL;
	memset(&data, 0, sizeof(data));
	config_foreach_section(config, section, _new_config_foreach_section,
			NULL);
	if(data.members == 0)
		appstatus->variable = NULL;
	else if((appstatus->variable = variable_new_compound_variables(NULL,
					data.members, data.names,
					data.variables)) == NULL)
	{
		appstatus_delete(appstatus);
		return NULL;
	}
	return appstatus;
}

static void _new_config_foreach_section(String const * variable,
		String const * value, void * data)
{
	struct
	{
		size_t members;
		String const ** names;
		Variable ** variables;
	} * d = data;

	/* FIXME implement */
}


/* appstatus_delete */
void appstatus_delete(AppStatus * appstatus)
{
	if(appstatus->variable != NULL)
		variable_delete(appstatus->variable);
	object_delete(appstatus);
}
