#!/bin/sh
#$Id$
#Copyright (c) 2012-2019 Pierre Pronchery <khorben@defora.org>
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
PROGNAME="tests.sh"
#executables
DATE="date"
DEBUG="_debug"
ECHO="echo"
UNAME="uname"
[ "$($UNAME -s)" != "Darwin" ] || ECHO="/bin/echo"


#functions
#date
_date()
{
	if [ -n "$SOURCE_DATE_EPOCH" ]; then
		TZ=UTC $DATE -d "@$SOURCE_DATE_EPOCH" '+%a %b %d %T %Z %Y'
	else
		$DATE
	fi
}


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

	_run "$@"
	res=$?
	if [ $res -ne 0 ]; then
		echo "./$test: $name: Failed with error code $res" >> "$target"
		echo " FAIL ($name, error $res)" 1>&2
	else
		echo " PASS" 1>&2
	fi
}


#run
_run()
{
	test="$1"
	name="$2"

	shift
	shift
	$ECHO -n "$test:" 1>&2
	(echo
	echo "Testing: $test $name ($OBJDIR)"
	[ -n "$OBJDIR" ] || OBJDIR="./"
	if [ -x "$OBJDIR$test" ]; then
		test="$OBJDIR$test"
	elif [ -x "./$test" ]; then
		test="./$test"
	fi
	LD_LIBRARY_PATH="$OBJDIR../src" $DEBUG "$test" "$@" 2>&1) >> "$target"
}


#test
_test()
{
	test="$1"
	name="$2"

	_run "$@"
	res=$?
	if [ $res -ne 0 ]; then
		echo "./$test: $name: Failed with error code $res" >> "$target"
		echo " FAIL" 1>&2
		FAILED="$FAILED $test($name, error $res)"
		return 2
	fi
	echo " PASS" 1>&2
}


#usage
_usage()
{
	echo "Usage: $PROGNAME [-c][-P prefix] target..." 1>&2
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
if [ $# -eq 0 ]; then
	_usage
	exit $?
fi

#XXX cross-compiling
[ -n "$PKG_CONFIG_SYSROOT_DIR" ] && exit 0

while [ $# -ne 0 ]; do
	target="$1"
	shift

	[ "$clean" -ne 0 ] && exit 0

	FAILED=
	echo "Performing tests:" 1>&2
	_date > "$target"
	_test "appbroker.sh" "Test" "Test.h"
	APPINTERFACE_Test=Test.interface \
		_test "appclient" "appclient" -a "Test" -n tcp:localhost:4242
	_test "appinterface" "appinterface" -a "Dummy"
	_test "appmessage" "appmessage"
	APPINTERFACE_Dummy=../data/Dummy.interface \
		_test "appserver" "appserver" -a "Dummy" -n tcp:localhost:4242
	APPINTERFACE_Dummy=../data/Dummy.interface \
		APPSERVER_Dummy="tcp:localhost:4242" \
		_test "appserver" "appserver" -a "Dummy"
	_test "includes" "includes"
	APPINTERFACE_Test=Test.interface \
		_test "lookup" "lookup Test tcp" -a "Test" \
		-n "tcp:localhost:4242"
	APPINTERFACE_Test=Test.interface \
		_test "lookup" "lookup Test tcp4" -a "Test" \
		-n "tcp4:localhost:4242"
	APPSERVER_Session="tcp:localhost:4242" _test "lookup" \
		"lookup Session" -a "Session"
	_test "pkgconfig.sh" "pkg-config"
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
	APPINTERFACE_Test=Test.interface \
		_fail "lookup" "lookup" -a "Test" -n "localhost"
	_fail "transport" "self" -p self
	_fail "transport" "tcp6 ::1:4242" -p tcp6 ::1:4242
	_fail "transport" "tcp ::1:4242" -p tcp ::1:4242
	_fail "transport" "udp6 ::1:4242" -p udp6 ::1:4242
	_fail "transport" "udp ::1:4242" -p udp ::1:4242
	if [ -n "$FAILED" ]; then
		echo "Failed tests:$FAILED" 1>&2
		exit 2
	fi
	echo "All tests completed" 1>&2
done
