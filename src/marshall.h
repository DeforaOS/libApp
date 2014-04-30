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



#ifndef LIBAPP_MARSHALL_H
# define LIBAPP_MARSHALL_H

# include <System/variable.h>
# include "App/app.h"


/* Marshall */
/* functions */
int marshall_call(App * app, VariableType type, Variable * result,
		void * (*func)(void *), size_t argc, Variable ** argv);

#endif /* !LIBAPP_MARSHALL_H */
