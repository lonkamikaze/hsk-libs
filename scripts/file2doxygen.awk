#!/usr/bin/awk -f
#
# This is a doxygen filter for unsupported scripting languages.
#
# It's pupose is to at least produce file documentation for those languages,
# as such it simply provides the file documentation at the beginning of
# a script (the comment after the shebang).
#

##
# Initialise file documentation.
#
!doc && /^#!/ {
	doc = 1
	print "/** \\file " FILENAME
	next
}

##
# Print file documentation.
#
doc && /^#/ {
	sub(/^# ?/, "")
	print
	next
}

##
# Mark the end of the documentation block.
#
doc {
	doc = 0
	print " */"
	next
}

