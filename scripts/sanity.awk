#!/usr/bin/awk -f

#
# Sanity checks for C functions and declarations.
#
# This program does not distinct between errors and bad style.
#
# @retval 0
#	No problems encountered
# @retval 1
#	Duplicated prototype
# @retval 2
#	Duplicated prototypes mismatching
# @retval 3
#	Prototype following function definition
# @retval 4
#	Function definition and prototype mismatch
# @retval 5
#	Function defined multiple times
#

BEGIN {
	# Get a unique temporary file
	cmd = "sh -c 'printf $$'"
	cmd | getline TMPFILE
	close(cmd)
	TMPFILE = "/tmp/sanity.awk." TMPFILE
	# Get cstrip cmd
	path = ENVIRON["LIBPROJDIR"]
	sub(/.+/, "&/", path)
	cmd = ARGV[0] " -f " path "scripts/cstrip.awk"
	for (i = 1; i < ARGC; i++) {
		cmd = cmd " '" ARGV[i] "' "
	}
	system(cmd "-DSDCC >" TMPFILE)
	delete ARGV
	ARGV[1] = TMPFILE
	ARGC = 2
}

/^#filename/ {
	filename = $2
	delete prototypes
	delete functions
	next
}

# Get prototypes
!/^(return|else|__sfr|__sfr16|__sbit) / && /[[:alnum:]_* ]+ [[:alnum:]_]+\(.*\)[[:alnum:]_* ]*;/ {
	declare = $0
	sub(/\(.*/, "", declare)
	sub(/.* /, "", declare)
	sub(/;/, "")
	if (prototypes[declare]) {
		print filename ": duplicated prototype " declare
		print "prototype:	" $0
		if (prototypes[declare] != $0) {
			print "mismatching:	" prototypes[declare]
			exit 2
		}
		exit 1
	}
	prototypes[declare] = $0
	if (functions[declare]) {
		print filename ": prototype for already defined function " declare
		print "prototype:	" $0
		print "function:	" functions[declare]
		exit 3
	}
	next
}

# Get definitions
!/^(else) / && /[[:alnum:]_* ]+ [[:alnum:]_]+\(.*\)[[:alnum:]_* ]*$/ {
	definition = $0
	sub(/\(.*/, "", definition)
	sub(/.* /, "", definition)
	if (prototypes[definition]) {
		if (prototypes[definition] != $0) {
			print filename ": function definition " definition " not matching previous prototype"
			print "definition:	" $0
			print "prototype:	" prototypes[definition]
			exit 4
		}
	}
	if (functions[definition]) {
		print filename ": duplicated function " definition
		print "function:	" $0
		exit 5
	}
	functions[definition] = $0
	next
}

END {
	# Stop writing to TMPFILE
	close(TMPFILE)
	cmd = "rm " TMPFILE
	system(cmd)
}

