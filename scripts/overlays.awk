#!/usr/bin/awk -f

# Finds call tree manipulations for µVision from C files.

BEGIN {RS = ";"}

{
	# Remove comments
	gsub(/\/\*[^*]*([^*]*\*[^\/])*\*\//, "")
	# Remove escaped newlines
	gsub(/\\[[:cntrl:]]/, "")
	# Remove spaces
	gsub(/[[:space:][:cntrl:]]+/, "")
	# Remove block open/close and jump symbols/case in front
	gsub(/^.*[{}:]/, "")
}

# Catch shared ISRs
/^hsk_isr[0-9]+\.[[:alnum:]]+=&[[:alnum:]_]+$/ {
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

END {
	# Group overlays
	for (overlay in overlays) {
		isr = overlay
		callback = overlay
		sub(/!.*/, "", isr)
		sub(/.*!/, "", callback)
		callbacks[isr] = (callbacks[isr] ? callbacks[isr] ", " : "") callback
	}

	# Print groups
	for (isr in callbacks) {
		printf (sep++ ? ",\r\n" : "") "* ~ (%s)", callbacks[isr]
		printf ",\r\n%s ! (%s)", isr, callbacks[isr]
	}
}

