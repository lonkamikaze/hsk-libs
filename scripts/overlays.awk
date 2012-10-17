#!/usr/bin/awk -f

# Finds call tree manipulations for µVision from C files.

BEGIN {
	RS = "\0"
	nested = 0
	# Get a unique temporary file
	cmd = "sh -c 'printf $$'"
	cmd | getline TMPFILE
	close(cmd)
	TMPFILE = "/tmp/overlays.awk." TMPFILE
	ARGV[ARGC++] = TMPFILE
}

#
# Accumulate and preprocess files so they become easier to parse
#
FILENAME != TMPFILE {
	# Preprocess file, so there is no trouble with unknown symbols
	cmd = (ENVIRON["CPP"] ? ENVIRON["CPP"] : "cpp") " -D__C51__ -DSDCC " FILENAME " 2> /dev/null"
	$0 = ""
	while (cmd | getline line) {
		$0 = $0 line ";"
	}
	close(cmd)
	# Remove comments
	gsub(/\/\*[^*]*(\*[^\/]|[^*])*\*\//, "")
	# Isolate preprocessor instructions
	gsub("(^|\n)[[:space:]]*#[^\n]*", "&;")
	# Remove escaped newlines
	gsub(/\\[[:cntrl:]]+/, "")
	# Collapse spaces
	gsub(/[[:space:][:cntrl:]]+/, " ")
	sub(/^ /, "")
	# Remove obsolete spaces
	split("()=!><|:?,*/+-%^~[].\"", ctrlchars, "")
	for (i in ctrlchars) {
		gsub(" \\" ctrlchars[i], ctrlchars[i])
		gsub("\\" ctrlchars[i] " ", ctrlchars[i])
	}
	gsub(" \\&", "\\&")
	gsub("\\& ", "\\&")
	# Segregate nested code
	gsub(/\{/, ";{;")
	gsub(/\}/, ";};")
	# Segregate labels
	gsub(/:/, ":;")
	# Trim
	gsub(/; /, ";")
	gsub(/ ;/, ";")
	# Remove empty lines
	gsub(/;+/, ";")
	gsub(/;+/, ";")
	# Store in TMPFILE
	printf "#filename %s;%s", FILENAME, $0 > TMPFILE

	# Switch to ; separation when processing the last file
	if (FILENAME == ARGV[ARGC - 2]) {
		RS = ";"
	}

	# Get next input line
	next
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
0 {
	printf "% " (nested * 8) "s%s\n", "", $0
	next
}

/^#filename/ {
	filename = $2
	next
}

# The hsk_isr_rootN() function is present, so an ISR call tree can be built.
/hsk_isr_root[0-9]+\(.*\)using[( ][0-9]+\)?/ {
	sub("^void ", "")
	sub(/\(.*/, "")
	roots[$0]
}

# Gather interrupts
/void .*\(void\)interrupt[( ][0-9]+[) ]using[( ][0-9]+\)?/ {
	sub(/^void /, "");
	isr = $0
	sub(/\(.*/, "", isr)
	sub(/.*using[( ]/, "")
	sub(/[^0-9].*/, "")
	using["hsk_isr_root" $0 "!" isr]
}

# Catch shared ISRs
/^hsk_isr[0-9]+\.[[:alnum:]_]+=&[[:alnum:]_]+$/ {
	sub(/^/, "ISR_")
	sub(/\..*=&/, "!")
	overlays[$0]
}

# Catch timer0/timer1 ISRs
/^hsk_timer[0-9]+_setup\(.*,&[[:alnum:]_]+\)$/ {
	sub(/^/, "ISR_")
	sub(/_setup\(.*,&/, "!")
	sub(/\)$/, "")
	overlays[$0]
}

# Catch external interrupts
/^hsk_ex_channel_enable\(.*,.*,&[[:alnum:]_]+\)$/ {
	chan = $0
	sub(/hsk_ex_channel_enable\([^0-9,]*/, "", chan)
	sub(/,.*/, "", chan)
	if (chan == 2) {
		chan = 8
	} else {
		chan = 9
	}
	if (chan >= 2) {
		sub(/.*,&/, "ISR_hsk_isr" chan "!")
		sub(/\)$/, "")
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

