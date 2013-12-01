#!/bin/sh -f
#
# This script produces a make file with build instructions for a given
# C source directory.
#
# @param 1
#	The source direcotry to generate build instrucitons for
# @param @
#	The remaining command line arguments are treated as include
#	directories
#
# This script performs two major steps:
# - Produce build instructions for each <tt>.c</tt> file
# - Produce linking instructions for each \c main function
#
# The output contains the meta target \c build, which links all programs.
# The meta target \c all simply builds everything, even targets not required
# for linking.
#
# \section env Environment Variables
#
# This script uses or sets certain environment variables:
#
# - AWK:
#	- This variable contains the AWK interpreter to use
#	- Defaults to \c awk
# - CPP:
#	- Used by some secondory scripts to process C code
#	- Not set by default
# - LIBPROJDIR:
#	- This variable is set to the relative path to the parent directory
#	  of the script and passed on to some AWK scripts
#	- Left empty if the script was called from the parent directory of
#	  the script
#
# \section calls Secondary Scripts
#
# This script calls other scripts during operation:
# - depends.awk
# - links.awk
#
# The following scripts are called from the generated make file
# - sanity.awk
#
# \section make Make Requirements
#
# The generated make file expects the following variables to be set:
# | Variable | Description
# |----------|-----------------------------------------------------------------
# | BUILDDIR | The directory to dump compiler output to
# | CC       | The C compiler
# | CFLAGS   | Compiler arguments
# | HEXSUFX  | The filename suffix for the hex file containing the linked code
# | OBJSUFX  | The filename suffix for object files
#

# Force noglob mode
set -f

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
	files="$(find "$SRC/" -name *.c)"
	
	# Build instructions
	for file in $files; do
		target="\${BUILDDIR}/${file#$SRC/}"
		target="${target%.c}\${OBJSUFX}"
		echo "$target" | grep -qFx "$all" && continue
		all="${all:+$all$IFS}$target"
		sources="$(IFS=' '
		           env LIBPROJDIR="$LIBPROJDIR" \
		               $AWK -vORS=\  -f $scriptdir/depends.awk \
		                    $file -DSDCC $incdirs)"
		echo "$target: $sources
	@mkdir -p ${target%/*}
	@env CPP=\"\${CC} -E\" ${LIBPROJDIR:+LIBPROJDIR=\"$LIBPROJDIR\"} $AWK -f $scriptdir/sanity.awk $file $incdirs
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
	linklist="$(IFS=' '
	            env LIBPROJDIR="$LIBPROJDIR" \
	                $AWK -f $scriptdir/links.awk $file -DSDCC $incdirs \
	            | sed -e "$filter" -e 's:\.c$:${OBJSUFX}:' \
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

