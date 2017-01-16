#!/bin/sh
#$Id$
#Copyright (c) 2016-2017 Pierre Pronchery <khorben@defora.org>
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
PROGNAME="appbroker.sh"
#executables
APPBROKER="$OBJDIR../tools/AppBroker$EXEEXT"
DIFF="diff"
DEBUG="_debug"
MKTEMP="mktemp"
RM="rm -f"


#functions
#main
tmpfile=$($MKTEMP)
[ $? -eq 0 ]							|| exit 2
interface="$1"
expected="${interface%.interface}.expected"
$APPBROKER -o "$tmpfile" "$interface"
ret=$?
if [ $ret -eq 0 ]; then
	$DIFF -- "$expected" "$tmpfile"
	ret=$?
fi
$RM -- "$tmpfile"
exit $ret
