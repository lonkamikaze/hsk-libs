#!/usr/bin/awk -f

# Finds alll includes of a C file

BEGIN {
	# Get environment settings
	DEBUG = ENVIRON["DEBUG"]
	LIBPROJDIR = ENVIRON["LIBPROJDIR"]
	sub(/[^\/]$/, "&/", LIBPROJDIR)
	if (DEBUG) {
		print "depends.awk: LIBPROJDIR = " LIBPROJDIR > "/dev/stderr"
	}

	# Get cstrip cmd
	cmd = ARGV[0] " -f " LIBPROJDIR "scripts/cstrip.awk"
	for (i = 1; i < ARGC; i++) {
		cmd = cmd " '" ARGV[i] "' "
	}
	delete ARGV

	# Handle cstrip output
	FS = "\""
	while (cmd | getline) {
		# Get file references, filter duplicates and skip builtins
		if ($0 ~ /#[0-9]+"/ && !a[$2]++ && $2 !~ /^<.*>$/) {
			print($2)
		}
	}
	close(cmd)
	exit 0
}

