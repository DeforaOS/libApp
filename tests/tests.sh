#!/usr/bin/env sh
#$Id$
#Copyright (c) 2012 Pierre Pronchery <khorben@defora.org>
#This file is part of DeforaOS System libApp
#This program is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, version 3 of the License.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program.  If not, see <http://www.gnu.org/licenses/>.


#functions
#transport
_transport()
{
	[ $# -eq 2 ]						|| return 1
	transport="$1"
	name="$2"

	echo "transport: Testing $transport ($name)" 1<&2
	./transport -p "$transport" "$name"
}


#usage
_usage()
{
	echo "Usage: tests.sh target" 1>&2
	return 1
}


#main
while getopts "P:" "name"; do
	case "$name" in
		P)
			#XXX ignored
			;;
		?)
			_usage
			exit $?
			;;
	esac
done
shift $((OPTIND - 1))
if [ $# -ne 1 ]; then
	_usage
	exit $?
fi
target="$1"

> "$target"
FAILED=
_transport tcp4 127.0.0.1:4242	>> "$target" || FAILED="$FAILED tcp4(error $?)"
_transport tcp6 ::1.4242	>> "$target" || FAILED="$FAILED tcp6(error $?)"
_transport tcp6 ::1:4242	>> "$target" || FAILED="$FAILED tcp6(error $?)"
_transport tcp  127.0.0.1:4242	>> "$target" || FAILED="$FAILED tcp(error $?)"
_transport tcp  ::1.4242	>> "$target" || FAILED="$FAILED tcp(error $?)"
_transport tcp  ::1:4242	>> "$target" || FAILED="$FAILED tcp(error $?)"
_transport udp4 127.0.0.1:4242	>> "$target" || FAILED="$FAILED udp4(error $?)"
_transport udp6 ::1.4242	>> "$target" || FAILED="$FAILED udp6(error $?)"
_transport udp  127.0.0.1:4242	>> "$target" || FAILED="$FAILED udp(error $?)"
_transport udp  ::1.4242	>> "$target" || FAILED="$FAILED udp(error $?)"
[ -z "$FAILED" ]			&& exit 0
echo "Failed tests:$FAILED" 1>&2
#XXX ignore errors for now
#exit 2
