#!/bin/sh -f
#
# Parses an sdcc config file.
#
# @param @
#	All arguments are treted as config files
#
# Expects \c CC in the environment.
# If \c CC does not refer to a version of SDCC, the script terminates with
# empty output.
#
# Configuration files contain make code and have sections, the first section
# is unconditional and thus always printed.
# The following sections are opened with a condition. Conditions stand in a
# single line using the following syntax:
# \verbatim
#	"[" condition "]"
# \endverbatim
#
# The string "SDCC" within the condition is replaced with the version of
# SDCC. The condition is then passed to the testver.sh script.
#

# Force noglob mode
set -f
LF='
'

scriptdir="${0%/*}"
compiler=$(eval $CC -v 2>/dev/null)

# Only take the first line of the compiler version string
compiler=${compiler%%$LF*}

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

