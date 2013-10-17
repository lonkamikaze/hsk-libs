#!/bin/sh -f

#
# Parses an sdcc config file.
#
# Expects CC in the environment.
#

scriptdir="${0%/*}"
compiler=$(eval $CC -v 2>/dev/null)

# Only configure SDCC
if ! echo "$compiler" | grep -qiF sdcc; then
	exit 0
fi

version=$(echo "$compiler" | cut -d\  -f4)

firstrun=1
while [ -n "${firstrun:-$1}" ]; do
	if [ -n "$1" ]; then
		echo "#$1"
		exec < "$1"
		shift
	fi

	# Print config file heading
	print=1
	while read line; do
		case "$line" in
		\[*\])
			# From the first conditional on only matching configurations
			# are printed.
			line=${line##*\[}
			line=${line%%]*}
			line=$(echo "${line%%]*}" | sed "s/SDCC/$version/g")
			print=
			if eval "$scriptdir/testver.sh $line"; then
				print=1
			fi
			;;
		*)
			test -n "$print" && echo "$line"
			;;
		esac
	done

	firstrun=
done

exit 0

