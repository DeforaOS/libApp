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
	#XXX
	protocol="$3"
	name="$4"

	shift
	echo -n "$test:" 1>&2
	echo
	echo "Testing: $test $protocol $name"
	$DEBUG "./$test" "$@" 2>&1
	res=$?
	if [ $res -ne 0 ]; then
		echo " FAILED ($protocol $name, error $res)" 1>&2
	else
		echo " PASS" 1>&2
	fi
}


#test
_test()
{
	test="$1"
	#XXX
	protocol="$3"
	name="$4"

	shift
	echo -n "$test:" 1>&2
	echo
	echo "Testing: $test $protocol $name"
	$DEBUG "./$test" "$@" 2>&1
	res=$?
	if [ $res -ne 0 ]; then
		echo " FAILED" 1>&2
		FAILED="$FAILED $test($protocol $name, error $res)"
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

[ "$clean" -ne 0 ]			&& exit 0

FAILED=
(echo "Performing tests:" 1>&2
$DATE
_test "appmessage"
_test "transport" -p tcp4 127.0.0.1:4242
_test "transport" -p tcp4 localhost:4242
_test "transport" -p tcp6 ::1.4242
_test "transport" -p tcp6 localhost:4242
_test "transport" -p tcp 127.0.0.1:4242
_test "transport" -p tcp ::1.4242
_test "transport" -p tcp localhost:4242
_test "transport" -p udp4 127.0.0.1:4242
_test "transport" -p udp4 localhost:4242
_test "transport" -p udp6 ::1.4242
_test "transport" -p udp6 localhost:4242
_test "transport" -p udp 127.0.0.1:4242
_test "transport" -p udp ::1.4242
_test "transport" -p udp localhost:4242
echo "Expected failures:" 1>&2
_fail "transport" -p tcp6 ::1:4242
_fail "transport" -p tcp ::1:4242
_fail "transport" -p udp6 ::1:4242
_fail "transport" -p udp ::1:4242) > "$target"
if [ -n "$FAILED" ]; then
	echo "Failed tests:$FAILED" 1>&2
	exit 2
fi
echo "All tests completed" 1>&2
