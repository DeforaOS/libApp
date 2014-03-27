/* $Id$ */
/* Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org> */
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



/* new_event_transport */
static AppTransport * _new_event_transport(AppTransportHelper * helper,
		Event * event, char const * app, char const * name)
{
	AppTransport * ret;
	String * n;
	String * transport;

	if((n = _new_server_name(app, name)) == NULL)
		return NULL;
	if((transport = _new_server_transport(&n)) == NULL)
	{
		string_delete(n);
		return NULL;
	}
	ret = apptransport_new(ATM_SERVER, helper, transport, n, event);
	string_delete(transport);
	string_delete(n);
	return ret;
}

static String * _new_server_name(char const * app, char const * name)
{
	String * var;

	if(name != NULL)
		return string_new(name);
	/* obtain the desired transport and name from the environment */
	if((var = string_new_append("APPSERVER_", app, NULL)) == NULL)
		return NULL;
	name = getenv(var);
	string_delete(var);
	return string_new(name);
}

static String * _new_server_transport(String ** name)
{
	String * p;
	String * transport;

	if((p = strchr(*name, ':')) == NULL)
		/* XXX hard-coded default value */
		return string_new("tcp");
	/* XXX */
	*(p++) = '\0';
	transport = *name;
	if((*name = string_new(p)) == NULL)
	{
		string_delete(transport);
		return NULL;
	}
	return transport;
}
