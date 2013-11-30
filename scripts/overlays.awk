#!/usr/bin/awk -f
#
# Finds call tree manipulations for µVision from C files.
#
# This script directly makes use of the coding conventions of the hsk_libs
# and uses internal knowledge, which makes it useless for any other
# purpose.
#

##
# Pass all arguments to cstrip.awk and pass the output to TMPFILE.
#
# Creates the following globals:
# - DEBUG: Created from the environment variable with the same name
# - LIBPROJDIR: Created from the environment variable with the same name
#   it is used to access cstrip.awk
# - TMPFILE: The file containing the output of cstrip.awk
#
BEGIN {
	# Get environment settings
	DEBUG = ENVIRON["DEBUG"]
	LIBPROJDIR = ENVIRON["LIBPROJDIR"]
	sub(/[^\/]$/, "&/", LIBPROJDIR)
	if (DEBUG) {
		print "overlays.awk: LIBPROJDIR = " LIBPROJDIR > "/dev/stderr"
	}

	nested = 0
	# Get a unique temporary file
	cmd = "sh -c 'printf $$'"
	cmd | getline TMPFILE
	close(cmd)
	TMPFILE = "/tmp/overlays.awk." TMPFILE
	# Get cstrip cmd
	cmd = ARGV[0] " -f " LIBPROJDIR "scripts/cstrip.awk"
	for (i = 1; i < ARGC; i++) {
		cmd = cmd " '" ARGV[i] "'"
	}
	system(cmd ">" TMPFILE)
	delete ARGV
	ARGV[1] = TMPFILE
	ARGC = 2

}

##
# Reduce nesting depth.
#
/\}/ {
	nested--
}

##
# Just for debugging level > 1, print the current input line.
#
DEBUG > 1 {
	printf "% " (nested * 8) "s%s\n", "", $0 > "/dev/stderr"
}

##
# Increase nesting depth.
#
/\{/ {
	nested++
}

##
# Get filename, useful for debugging.
#
/^#[0-9]+".*"/ {
	sub(/^#[0-9]+"/, "");
	sub(/".*/, "");
	filename = $0
	next
}

##
# The hsk_isr_rootN() function is present, so an ISR call tree can be built.
#
/^void hsk_isr_root[0-9]+\(.*\)(__)?using [0-9]+$/ {
	sub("^void ", "")
	sub(/\(.*/, "")
	roots[++roots_i] = $0
}

##
# Gather interrupts.
#
/^void .*\(void\)(__)?interrupt [0-9]+ (__)?using [0-9]+$/ {
	if (DEBUG) {
		print "overlays.awk: ISR: " $0 > "/dev/stderr"
	}
	sub(/^void /, "");
	isr = $0
	sub(/\(.*/, "", isr)
	sub(/.*(__)?using[( ]/, "")
	sub(/[^0-9].*/, "")
	using[++using_i] = "hsk_isr_root" $0 "!" isr
}

##
# Catch shared ISRs.
#
/^hsk_isr[0-9]+\.[a-zA-Z0-9_]+=&[a-zA-Z0-9_]+;/ {
	if (DEBUG) {
		print "overlays.awk: shared ISR: " $0 > "/dev/stderr"
	}
	sub(/^/, "ISR_")
	sub(/\..*=&/, "!")
	sub(/;/, "")
	overlays[++overlays_i] = $0
}

##
# Catch timer0/timer1 ISRs.
#
/^hsk_timer[0-9]+_setup\(.*,\&[a-zA-Z0-9_]+\);/ {
	if (DEBUG) {
		print "overlays.awk: timer01 ISR: " $0 > "/dev/stderr"
	}
	sub(/^/, "ISR_")
	sub(/_setup\(.*,&/, "!")
	sub(/\);/, "")
	overlays[++overlays_i] = $0
}

##
# Catch external interrupts.
#
/^hsk_ex_channel_enable\([a-zA-Z0-9]+,.*,&[a-zA-Z0-9_]+\);/ {
	if (DEBUG) {
		print "overlays.awk: ext ISR: " $0 > "/dev/stderr"
	}
	chan = $0
	sub(/hsk_ex_channel_enable\(/, "", chan)
	sub(/,.*/, "", chan)
	if (chan == 2) {
		chan = 8
	} else if (chan > 2) {
		chan = 9
	}
	if (chan >= 2) {
		sub(/.*,&/, "ISR_hsk_isr" chan "!")
		sub(/\);/, "")
		overlays[++overlays_i] = $0
	}
}

##
# Remove TMPFILE and print assembled data.
#
END {
	# Stop writing to TMPFILE
	close(TMPFILE)
	cmd = "rm " TMPFILE
	system(cmd)

	# Group overlays
	i = 0
	while (i++ < overlays_i) {
		isr = overlays[i]
		callback = overlays[i]
		sub(/!.*/, "", isr)
		sub(/.*!/, "", callback)
		# Fix order of isr callback groups
		if (!callbacks[isr]) {
			groups[++groups_i] = isr
		}
		# Avoid duplicates
		if (!callback_count[callback]++) {
			callbacks[isr] = (callbacks[isr] ? callbacks[isr] ", " : "") callback
		}
	}

	# Group ISRs
	i = 0
	while (i++ < using_i) {
		isr = using[i]
		root = using[i]
		sub(/.*!/, "", isr)
		sub(/!.*/, "", root)
		isrs[root] = (isrs[root] ? isrs[root] ", " : "") isr
	}

	# Print ISR groups
	i = 0
	while (i++ < roots_i) {
		printf (sep++ ? ",\r\n" : "") "* ! (%s)", roots[i]
		printf ",\r\n%s ! (%s)", roots[i], isrs[roots[i]]
	}

	# Print groups
	i = 0
	while (i++ < groups_i) {
		if (!severed[callbacks[groups[i]]]++) {
			printf (sep++ ? ",\r\n" : "") "* ~ (%s)", callbacks[groups[i]]
		}
	}
	i = 0
	while (i++ < groups_i) {
		printf ",\r\n%s ! (%s)", groups[i], callbacks[groups[i]]
	}
}

