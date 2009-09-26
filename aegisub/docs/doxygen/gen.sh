#!/bin/sh
#
# $Id$
#

if test -z "$1" || test -z "$2"; then
	echo "You must provide a project and output dir."
	exit;
fi

case "$1" in
	"aegisub")
		TRIM="${SRC_PWD}/src/"
	;;
	"reporter")
		TRIM="${SRC_PWD}/reporter/"
	;;
esac

export OUTPUT_DIR="$2"
export SRC_TRIM="${TRIM}"


mkdir -vp "$2"
cp -v css.css "$2"

SRC_PWD=`pwd|sed "s|/docs/doxygen||"`

doxygen "doxyfile_$1"
