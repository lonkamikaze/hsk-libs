#!/usr/bin/awk -f

# Finds call tree manipulations for µVision from C files.

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
		cmd = cmd " '" ARGV[i] "' "
	}
	system(cmd ">" TMPFILE)
	delete ARGV
	ARGV[1] = TMPFILE
	ARGC = 2

}

/\}/ {
	nested--
}

# Just for debugging
DEBUG > 1 {
	printf "% " (nested * 8) "s%s\n", "", $0 > "/dev/stderr"
}

/\{/ {
	nested++
}

# Get filename, useful for debugging
/^#[0-9]+".*"/ {
	sub(/^#[0-9]+"/, "");
	sub(/".*/, "");
	filename = $0
	next
}

# The hsk_isr_rootN() function is present, so an ISR call tree can be built.
/^void hsk_isr_root[0-9]+\(.*\)(__)?using [0-9]+$/ {
	sub("^void ", "")
	sub(/\(.*/, "")
	roots[$0]
}

# Gather interrupts
/^void .*\(void\)(__)?interrupt [0-9]+ (__)?using [0-9]+$/ {
	if (DEBUG) {
		print "overlays.awk: ISR: " $0 > "/dev/stderr"
	}
	sub(/^void /, "");
	isr = $0
	sub(/\(.*/, "", isr)
	sub(/.*(__)?using[( ]/, "")
	sub(/[^0-9].*/, "")
	using["hsk_isr_root" $0 "!" isr]
}

# Catch shared ISRs
/^hsk_isr[0-9]+\.[a-zA-Z0-9_]+=&[a-zA-Z0-9_]+;/ {
	if (DEBUG) {
		print "overlays.awk: shared ISR: " $0 > "/dev/stderr"
	}
	sub(/^/, "ISR_")
	sub(/\..*=&/, "!")
	sub(/;/, "")
	overlays[$0]
}

# Catch timer0/timer1 ISRs
/^hsk_timer[0-9]+_setup\(.*,\&[a-zA-Z0-9_]+\);/ {
	if (DEBUG) {
		print "overlays.awk: timer01 ISR: " $0 > "/dev/stderr"
	}
	sub(/^/, "ISR_")
	sub(/_setup\(.*,&/, "!")
	sub(/\);/, "")
	overlays[$0]
}

# Catch external interrupts
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
		overlays[$0]
	}
}

END {
	# Stop writing to TMPFILE
	close(TMPFILE)
	cmd = "rm " TMPFILE
	system(cmd)

	# Group overlays
	for (overlay in overlays) {
		isr = overlay
		callback = overlay
		sub(/!.*/, "", isr)
		sub(/.*!/, "", callback)
		callbacks[isr] = (callbacks[isr] ? callbacks[isr] ", " : "") callback
	}

	# Group ISRs
	for (overlay in using) {
		isr = overlay
		root = overlay
		sub(/.*!/, "", isr)
		sub(/!.*/, "", root)
		isrs[root] = (isrs[root] ? isrs[root] ", " : "") isr
	}

	# Print ISR groups
	for (root in roots) {
		printf (sep++ ? ",\r\n" : "") "* ! (%s)", root
		printf ",\r\n%s ! (%s)", root, isrs[root]
	}

	# Print groups
	for (isr in callbacks) {
		if (!severed[callbacks[isr]]++) {
			printf (sep++ ? ",\r\n" : "") "* ~ (%s)", callbacks[isr]
		}
	}
	for (isr in callbacks) {
		printf ",\r\n%s ! (%s)", isr, callbacks[isr]
	}
}

