#!/bin/sh -f
#
# This script produces a make file with instructions to generate C headers
# from Vector DBC files.
#
# @param @
#	The directories containing <tt>.dbc</tt> files
#
# The script uses the \c LIBPROJDIR and \c AWK environment variables similar
# to the way build.sh uses them.
#
# For each <tt>.dbc</tt> file a target to create an UTF-8 version of the file
# is created. The file is expected to use CP1252 (Windows encoding), the
# conversion is performed by \c iconv.
#
# The next target uses dbc2c.awk to generate the desired C headers.
#
# The meta target \c dbc generates all headers.
#

# Force noglob mode
set -f

IFS='
'

scriptdir="${0%/*}"
LIBPROJDIR="${scriptdir##*/}"
LIBPROJDIR="${scriptdir%$LIBPROJDIR}"
LIBPROJDIR="${LIBPROJDIR%/}"

# AWK interpreter
: ${AWK:=awk}

all=
for SRC in "$@"; do
	SRC="${SRC%/}"
	for dbc in $(find "$SRC/" -name *.dbc); do
		encoded="\${DBCDIR}/${dbc#$SRC/}"
		target="${encoded%.*}.h"
		all="$all $target"
		echo "$encoded: $dbc
	@mkdir -p ${target%/*}
	iconv -f CP1252 -t UTF-8 $dbc > $encoded

$target: $encoded
	${LIBPROJDIR:+env LIBPROJDIR=$LIBPROJDIR} $AWK -f $scriptdir/dbc2c.awk $encoded > $target
"
	done
done

echo "\${DBCDIR}:
	@mkdir -p \${DBCDIR}
"

echo ".PHONY: dbc"
echo "dbc: $all"

