#!/bin/sh -f

IFS='
'

scriptdir="${0%/*}"

find -s "$@" -name \*.c | xargs awk -f $scriptdir/includes.awk "$@"

all=
for SRC in "$@"; do
	SRC="${SRC%/}"
	# Collect dependencies for the build target
	files="$(find -s "$SRC" -name \*.c)"
	
	# Build instructions
	for file in $files; do
		target="\${BUILDDIR}/${file#$SRC/}"
		target="${target%.c}\${OBJSUFX}"
		echo "$target" | grep -qFx "$all" && continue
		all="${all:+$all$IFS}$target"
		echo "$target: $file
	@mkdir -p ${target%/*}
	@env LIBPROJDIR=\${LIBPROJDIR} awk -f $scriptdir/sanity.awk $file -I\${LIBDIR} -I\${INCDIR} -I\${CANDIR}
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
	linklist="$(awk -f $scriptdir/includes.awk "$@" $file | cut -d: -f1 \
		| sed -ne "$filter" -e "s:\.c\$:\${OBJSUFX}:p" \
		| rs -TC\  )"

	echo "$target: $linklist
	@mkdir -p ${target%/*}
	\${CC} \${CFLAGS} -o $target $linklist
"
done

printf "all:"
for target in $all; do
	printf " $target"
done
echo

echo "build:$build"

