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



/* init_address */
static struct addrinfo * _init_address(char const * name, int domain, int flags)
{
	struct addrinfo * ai = NULL;
	char sep = ':';
	char * p;
	char * q;
	char * r;
	long l;
	struct addrinfo hints;
	int res = 0;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %d, %d)\n", __func__, name, domain,
			flags);
#endif
	/* guess the port separator */
	if((q = strchr(name, ':')) != NULL && strchr(++q, ':') != NULL)
		sep = '.';
	/* obtain the name */
	if(strlen(name) == 0)
		p = NULL;
	else if((p = strdup(name)) == NULL)
	{
		error_set_code(1, "%s", strerror(errno));
		return NULL;
	}
	/* obtain the port number */
	if(p == NULL || (q = strrchr(p, sep)) == NULL)
	{
		l = 0;
		q = NULL;
	}
	else
	{
		*(q++) = '\0';
		l = strtol(q, &r, 10);
		if(q[0] == '\0' || *r != '\0')
			l = -error_set_code(1, "%s", strerror(EINVAL));
	}
	/* FIXME perform this asynchronously */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = domain;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = flags;
	if(l >= 0)
		res = getaddrinfo(p, q, &hints, &ai);
	free(p);
	/* check for errors */
	if(res != 0)
	{
		error_set_code(1, "%s", gai_strerror(res));
		if(ai != NULL)
			freeaddrinfo(ai);
		return NULL;
	}
	return ai;
}
