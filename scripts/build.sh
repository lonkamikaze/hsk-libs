#!/bin/sh -f

IFS='
'

scriptdir="${0%/*}"
LIBPROJDIR="${scriptdir##*/}"
LIBPROJDIR="${scriptdir%$LIBPROJDIR}"
LIBPROJDIR="${LIBPROJDIR%/}"

incdirs=
for dir in ${*#$1}; do
	incdirs="$incdirs${incdirs:+ }-I$dir"
done

# AWK interpreter
: ${AWK:=awk}

all=
for SRC in "$@"; do
	SRC="${SRC%/}"
	# Collect dependencies for the build target
	files="$(find "$SRC/" -name \*.c)"
	
	# Build instructions
	for file in $files; do
		target="\${BUILDDIR}/${file#$SRC/}"
		target="${target%.c}\${OBJSUFX}"
		echo "$target" | grep -qFx "$all" && continue
		all="${all:+$all$IFS}$target"
		sources="$(env LIBPROJDIR="$LIBPROJDIR" \
		               $AWK -f $scriptdir/depends.awk -vORS=\  \
		                    $file -DSDCC $incdirs)"
		echo "$target: $sources
	@mkdir -p ${target%/*}
	@env CPP=\"\${CPP}\" LIBPROJDIR=\"\${LIBPROJDIR}\" $AWK -f $scriptdir/sanity.awk $file -DSDCC $incdirs
	\${CC} \${CFLAGS} -o $target -c $file
	"
	done

done

# Link instructions
files="$(grep -lr '[^[:alnum:]]main[[:space:]]*(' "$1" | grep '\.c$')"
build=

for file in $files; do
	target="$file"
	filter=
	for SRC in "$@"; do
		SRC="${SRC%/}"
		target="${target#$SRC/}"
		filter="$filter${filter:+$IFS}s:^$SRC:\${BUILDDIR}:"
	done
	target="\${BUILDDIR}/${target%.c}\${HEXSUFX}"
	echo "$target" | grep -qFx "$all" && continue
	all="${all:+$all$IFS}$target"
	build="$build $target"
	linklist="$($AWK -f $scriptdir/includes.awk "$@" $file | cut -d: -f1 \
		| sed -ne "$filter" -e 's:\.c$:${OBJSUFX}:p' \
		| $AWK -vORS=\  1)"

	echo "$target: $linklist
	@mkdir -p ${target%/*}
	\${CC} \${CFLAGS} -o $target $linklist
"
done

echo ".PHONY: all build"
printf "all:"
for target in $all; do
	printf " $target"
done
echo

echo "build:$build"

