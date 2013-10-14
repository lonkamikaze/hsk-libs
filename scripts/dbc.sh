#!/bin/sh -f

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
	for dbc in $(find "$SRC/" -name \*.dbc); do
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

echo "dbc: $all"

