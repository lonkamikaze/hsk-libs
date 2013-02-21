#!/usr/bin/awk -f

# Finds call tree manipulations for µVision from C files.

BEGIN {
	nested = 0
	# Get a unique temporary file
	cmd = "sh -c 'printf $$'"
	cmd | getline TMPFILE
	close(cmd)
	TMPFILE = "/tmp/overlays.awk." TMPFILE
	# Get cscript cmd
	path = ENVIRON["LIBPROJDIR"]
	sub(/.+/, "&/", path)
	cmd = ARGV[0] " -f " path "scripts/cstrip.awk"
	for (i = 1; i < ARGC; i++) {
		cmd = cmd " '" ARGV[i] "' "
	}
	system(cmd ">" TMPFILE)
	delete ARGV
	ARGV[1] = TMPFILE
	ARGC = 2
}

/\{/ {
	nested++
	next
}

/\}/ {
	nested--
	next
}

# Just for debugging
ENVIRON["DEBUG"] {
	printf "% " (nested * 8) "s%s\n", "", $0 > "/dev/stderr"
}

# The hsk_isr_rootN() function is present, so an ISR call tree can be built.
/^void hsk_isr_root[0-9]+\(.*\)(__)?using [0-9]+$/ {
	sub("^void ", "")
	sub(/\(.*/, "")
	roots[$0]
}

# Gather interrupts
/^void .*\(void\)(__)?interrupt [0-9]+ (__)?using [0-9]+$/ {
	sub(/^void /, "");
	isr = $0
	sub(/\(.*/, "", isr)
	sub(/.*(__)?using[( ]/, "")
	sub(/[^0-9].*/, "")
	using["hsk_isr_root" $0 "!" isr]
}

# Catch shared ISRs
/^hsk_isr[0-9]+\.[[:alnum:]_]+=&[[:alnum:]_]+;/ {
	sub(/^/, "ISR_")
	sub(/\..*=&/, "!")
	sub(/;/, "")
	overlays[$0]
}

# Catch timer0/timer1 ISRs
/^hsk_timer[0-9]+_setup\(.*,&[[:alnum:]_]+\);/ {
	sub(/^/, "ISR_")
	sub(/_setup\(.*,&/, "!")
	sub(/\);/, "")
	overlays[$0]
}

# Catch external interrupts
/^hsk_ex_channel_enable\([[:alnum:]]+,.*,&[[:alnum:]_]+\);/ {
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

