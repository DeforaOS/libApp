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



#include <dlfcn.h>
#include <System.h>
#include "App/appmessage.h"
#include "App/apptransport.h"


/* Self */
/* private */
/* types */
typedef struct _AppTransportPlugin Self;

struct _AppTransportPlugin
{
	AppTransportPluginHelper * helper;
	AppTransportMode mode;

	void * dl;
};


/* protected */
/* prototypes */
/* plug-in */
static Self * _self_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name);
static void _self_destroy(Self * self);


/* public */
/* constants */
/* plug-in */
AppTransportPluginDefinition transport =
{
	"Self",
	NULL,
	_self_init,
	_self_destroy,
	/* FIXME implement the missing callbacks */
	NULL,
	NULL
};


/* protected */
/* functions */
/* plug-in */
/* self_init */
static Self * _self_init(AppTransportPluginHelper * helper,
		AppTransportMode mode, char const * name)
{
	Self * self;

	if((self = object_new(sizeof(*self))) == NULL)
		return NULL;
	self->helper = helper;
	self->mode = mode;
	if((self->dl = dlopen(NULL, RTLD_LAZY)) == NULL)
	{
		_self_destroy(self);
		return NULL;
	}
	return self;
}


/* self_destroy */
static void _self_destroy(Self * self)
{
	if(self->dl != NULL)
		dlclose(self->dl);
	object_delete(self);
}
