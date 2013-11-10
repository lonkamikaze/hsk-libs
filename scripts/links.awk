#!/usr/bin/awk -f

# Finds all C files whose objects need to be linked

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
	pass = "("
	for (i = 1; i < ARGC; i++) {
		if (ARGV[i] == "-I") {
			delete ARGV[i++]
			ARGV[i] = "-I" ARGV[i]
		}
		path = ""
		if (ARGV[i] ~ /^-I.+/) {
			path = ARGV[i]
			sub(/^-I/, "", path)
			sub(/\/?$/, "/", path)
		} else if (ARGV[i] !~ /^-/) {
			path = ARGV[i]
			sub(/\/[^\/]*$/, "/", path)
		}
		if (path) {
			# Remove ../
			while(sub(/[^\/]+\/\.\.\//, "", path));
			pass = pass "^" path "|"
		}
		cmd = cmd " '" ARGV[i] "' "
	}
	sub(/\|$/, ")", pass)
	delete ARGV
	if (DEBUG) {
		print "depends.awk: pass = " pass > "/dev/stderr"
	}

	# Handle cstrip output
	FS = "\""
	while (cmd | getline) {
		# Get file references, filter duplicates and skip builtins
		if ($0 ~ /#[0-9]+"/) {
			# Remove ../
			while(sub(/[^\/]+\/\.\.\//, "", $2));
			# Only objects created from .c files need
			# to be linked
			sub(/\.[^.]*$/, ".c", $2)
			if (!a[$2]++ && $2 ~ pass && !system("test -f " $2)) {
				print($2)
			}
		}
	}
	close(cmd)
	exit 0
}

