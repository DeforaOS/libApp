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



#include <string.h>
#include <errno.h>
#include <System/error.h>
#include "marshall.h"


/* Marshall */
/* public */
/* functions */
static int _call0(VariableType type, Variable * result, void * func);

int marshall_call(VariableType type, Variable * result, void * func,
		size_t argc, Variable ** argv)
{
	if(argc == 0)
		return _call0(type, result, func);
	/* FIXME implement */
	return -1;
}

static int _call0(VariableType type, Variable * result, void * func)
{
	int ret = 0;
	union
	{
		void * call;
		void (*call_null)(void);
		uint8_t (*call_bool)(void);
		int8_t (*call_i8)(void);
		uint8_t (*call_u8)(void);
		int16_t (*call_i16)(void);
		uint16_t (*call_u16)(void);
		int32_t (*call_i32)(void);
		uint32_t (*call_u32)(void);
		int32_t (*call_i64)(void);
		uint32_t (*call_u64)(void);
		float (*call_f)(void);
		double (*call_d)(void);
	} f;
	union
	{
		uint8_t b;
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int32_t i64;
		uint32_t u64;
		float f;
		double d;
	} res;

	/* FIXME implement through the generic marshaller instead */
	f.call = func;
	switch(type)
	{
		case VT_NULL:
			f.call_null();
			break;
		case VT_BOOL:
			res.b = (f.call_bool() != 0) ? 1 : 0;
			if(result != NULL)
				ret = variable_set_from(result, type, &res.b);
			break;
		case VT_INT8:
			res.i8 = f.call_i8();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.i8);
			break;
		case VT_UINT8:
			res.u8 = f.call_u8();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.u8);
			break;
		case VT_INT16:
			res.i16 = f.call_i16();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.i16);
			break;
		case VT_UINT16:
			res.u16 = f.call_u16();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.u16);
			break;
		case VT_INT32:
			res.i32 = f.call_i32();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.i32);
			break;
		case VT_UINT32:
			res.u32 = f.call_u32();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.u32);
			break;
		case VT_INT64:
			res.i64 = f.call_i64();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.i64);
			break;
		case VT_UINT64:
			res.u64 = f.call_u64();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.u64);
			break;
		case VT_FLOAT:
			res.f = f.call_f();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.f);
			break;
		case VT_DOUBLE:
			res.d = f.call_d();
			if(result != NULL)
				ret = variable_set_from(result, type, &res.d);
			break;
		default:
			return -error_set_code(1, "%s", strerror(ENOSYS));
	}
	return ret;
}
