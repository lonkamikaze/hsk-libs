#!/bin/sh -f
#
# Implements comparison of version numbers.
#
# This works by splitting the digits at the ".". Any operator accepted by
# test(1) works, as long as the operator is applicable to the digit.
# Missing digits (present in the other operand) are assumed 0.
#
# @param 1
#	First version number
# @param 2
#	Comparison operator
# @param 3
#	Second version number
#

# Force noglob mode
set -f

a=$1
operator=$2
b=$3

while [ -n "$a$b" ]; do
	# Fetch the most significant digits
	va=${a%%.*}
	va=${va:-0}
	vb=${b%%.*}
	vb=${vb:-0}

	# When equal always defer to the next subversion
	if [ "$va" != "$vb" ]; then
		test $va "$operator" $vb
		exit
	fi

	# Remove the most significant digit
	a=${a#$va}
	a=${a#.}
	b=${b#$vb}
	b=${b#.}
done

# All digits were equal, so just feed the operator with two equal values
test 0 "$operator" 0
exit

