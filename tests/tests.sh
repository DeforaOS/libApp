#!/bin/sh
#$Id$
#Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org>
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


#variables
#executables
DATE="date"
DEBUG="_debug"


#functions
#debug
_debug()
{
	echo "$@" 1>&2
	"$@"
}


#fail
_fail()
{
	test="$1"
	name="$2"

	shift
	shift
	echo -n "$test:" 1>&2
	(echo
	echo "Testing: $test $name"
	$DEBUG "./$test" "$@" 2>&1) >> "$target"
	res=$?
	if [ $res -ne 0 ]; then
		echo "./$test: $name: Failed with error code $res" >> "$target"
		echo " FAILED ($name, error $res)" 1>&2
	else
		echo " PASS" 1>&2
	fi
}


#test
_test()
{
	test="$1"
	name="$2"

	shift
	shift
	echo -n "$test:" 1>&2
	(echo
	echo "Testing: $test $name"
	$DEBUG "./$test" "$@" 2>&1) >> "$target"
	res=$?
	if [ $res -ne 0 ]; then
		echo "./$test: $name: Failed with error code $res" >> "$target"
		echo " FAILED" 1>&2
		FAILED="$FAILED $test($name, error $res)"
		return 2
	fi
	echo " PASS" 1>&2
}


#usage
_usage()
{
	echo "Usage: tests.sh [-c][-P prefix] target" 1>&2
	return 1
}


#main
clean=0
while getopts "cP:" name; do
	case "$name" in
		c)
			clean=1
			;;
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

[ "$clean" -ne 0 ] && exit 0

FAILED=
echo "Performing tests:" 1>&2
$DATE > "$target"
_test "appmessage" "appmessage"
_test "lookup" "lookup VFS tcp" -a "VFS" -n "tcp:localhost:4242"
_test "lookup" "lookup VFS tcp4" -a "VFS" -n "tcp4:localhost:4242"
#XXX avoid the export/unset dance
export APPSERVER_Session="tcp:localhost:4242"
_test "lookup" "lookup Session" -a "Session"
unset APPSERVER_Session
_test "transport" "tcp4 127.0.0.1:4242" -p tcp4 127.0.0.1:4242
_test "transport" "tcp4 localhost:4242" -p tcp4 localhost:4242
_test "transport" "tcp6 ::1.4242" -p tcp6 ::1.4242
_test "transport" "tcp6 localhost:4242" -p tcp6 localhost:4242
_test "transport" "tcp 127.0.0.1:4242" -p tcp 127.0.0.1:4242
_test "transport" "tcp ::1.4242" -p tcp ::1.4242
_test "transport" "tcp localhost:4242" -p tcp localhost:4242
_test "transport" "udp4 127.0.0.1:4242" -p udp4 127.0.0.1:4242
_test "transport" "udp4 localhost:4242" -p udp4 localhost:4242
_test "transport" "udp6 ::1.4242" -p udp6 ::1.4242
_test "transport" "udp6 localhost:4242" -p udp6 localhost:4242
_test "transport" "udp 127.0.0.1:4242" -p udp 127.0.0.1:4242
_test "transport" "udp ::1.4242" -p udp ::1.4242
_test "transport" "udp localhost:4242" -p udp localhost:4242
echo "Expected failures:" 1>&2
#XXX appclient, appserver and lookup should really succeed
_fail "appclient" "appclient" -a "VFS" -n "tcp:"
_fail "appserver" "appserver" -a "VFS" -n "tcp:"
_fail "lookup" "lookup" -a "VFS" -n "localhost"
_fail "transport" "tcp6 ::1:4242" -p tcp6 ::1:4242
_fail "transport" "tcp ::1:4242" -p tcp ::1:4242
_fail "transport" "udp6 ::1:4242" -p udp6 ::1:4242
_fail "transport" "udp ::1:4242" -p udp ::1:4242
if [ -n "$FAILED" ]; then
	echo "Failed tests:$FAILED" 1>&2
	exit 2
fi
echo "All tests completed" 1>&2
