#!/bin/sh -f

SRC="${1%/}"
IFS='
'

# Collect dependencies for the build target
all=
files="$(find -s "$SRC" -name \*.c)"

# Dependencies
awk -f scripts/includes.awk $files

# Build instructions
for file in $files; do
	target="\${BUILDDIR}/${file#$SRC/}"
	target="${target%.c}\${OBJSUFX}"
	all="${all:+$all }$target"
	echo "$target: $file
	@mkdir -p ${target%/*}
	\${CC} \${CFLAGS} -o $target -c $file
"
done

# Link instructions
files="$(grep -lr '[^[:alnum:]]main[[:space:]]*(' "$SRC" | grep '\.c$')"
for file in $files; do
	linklist="$(awk -f scripts/includes.awk $file | cut -d: -f1 \
		| sed -ne "s:^$SRC:\${BUILDDIR}:" -e "s:\.c\$:\${OBJSUFX}:p" \
		| rs -TC\  )"
	target="\${BUILDDIR}/${file#$SRC/}"
	target="${target%.c}\${HEXSUFX}"
	all="${all:+$all }$target"

	echo "$target: $linklist
	@mkdir -p ${target%/*}
	\${CC} \${CFLAGS} -o $target $linklist
"
done

echo "build: $all"

