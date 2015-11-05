/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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



#ifndef LIBAPP_APPSTATUS_H
# define LIBAPP_APPSTATUS_H

# include "App/appstatus.h"


/* AppStatus */
/* functions */
AppStatus * appstatus_new_config(Config * config, String const * section);
void appstatus_delete(AppStatus * appstatus);

/* accessors */
int appstatus_set(AppStatus * appstatus, String const * name, void * value);

#endif /* !LIBAPP_APPSTATUS_H */
