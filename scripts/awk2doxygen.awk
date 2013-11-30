#!/usr/bin/awk -f
#
# Converts an awk script containing doxygen comments into something the
# C parser of doxygen can handle.
#
# @warning
#	This script only produces correct output for completely documented
#	code.
#

##
# Initialise globals.
#
BEGIN {
	DEBUG = ENVIRON["DEBUG"]
	lastprint = 0
}

##
# Prints a debugging message on stderr.
#
# The debugging message is only printed if DEBUG is set.
#
# @param msg
#	The message to print
#
function debug(msg) {
	if (DEBUG) {
		print "awk2doxygen.awk: " msg > "/dev/stderr"
	}
}

##
# Fill the input buffer.
#
# Buffering is used to inject code into previous output lines.
#
{
	buf[++line] = $0
	while (lastprint < line - 100) {
		print(buf[++lastprint])
		delete buf[lastprint]
	}
}

##
# Setup globals to assemble a new documentation block.
#
# The following globals are reset:
# - doc(string): Set to enter documentation mode, contains the initial
#   string to be able to reproduce the indention
# - fret(bool): The coming function returns something
# - fargs(array): The coming function has the following arguments,
#   further arguments are discarded as local variables
#
function initDoc() {
	doc = $0
	delete fargs
	fret = 0
}

##
# Generates a function signature.
#
# @param name
#	The function name
# @return
#	A string containing a function declaration
#
function genFunction(name,
	args, i) {
	i = 0
	args = ""
	while (fargs[i]) {
		args = args ", var " fargs[i++]
	}
	sub(/^, /, "", args)
	return (fret ? "var " : "void ") name "(" args ")"
}

##
# Flush the remaining input buffer.
#
END {
	while (lastprint < line) {
		print(buf[++lastprint])
	}
	delete buf
}

##
# Initialise file documentation.
#
!doc && /^#!/ {
	initDoc()
	buf[line] = "/** \\file " FILENAME
	next
}

##
# Initialise documentation block.
#
!doc && /^[ \t]*##/ {
	initDoc()
	sub(/##/, "/**", buf[line])
	next
}

##
# Close documentation block.
#
# This closes a documentation block and generates a function signature
# for functions and filters.
#
doc && /^[ \t]*([^#]|$)/ {
	# Generate correctly indented closing block
	sub(/#.*/, " */", doc)
	# Insert closing block
	if (line > 1 && buf[line - 1] == "") {
		buf[line - 1] = doc
	} else {
		buf[line + 1] = buf[line]
		buf[line++] = doc
	}
	# Unset documentation mode
	doc = 0

	# Join concatenated lines after a documentation block
	while (buf[line] ~ /\\$/) {
		sub(/\\$/, "", buf[line])
		getline
		buf[line] = buf[line] $0
	}

	condition = ""
	# Functions
	if (buf[line] ~ /^[ \t]*function[ \t]/) {
		while (buf[line] !~ /{/) {
			getline
			buf[line] = buf[line] $0
		}
		name = buf[line]
		sub(/^[ \t]*function[ \t]+/, "", name)
		sub(/[ \t]*\(.*/, "", name)
		sub(/function[ \t][^{]*{/, genFunction(name) " {", buf[line])
	}
	# Unconditional filter
	else if (buf[line] ~ /^[ \t]*{/) {
		sub(/{/, "void filter" NR "() {", buf[line])
	}
	# Conditional filter
	else if (buf[line] ~ /{/) {
		condition = buf[line]
		sub(/{.*/, "", condition)
		sub(/.*{/, "void filter" NR "() {", buf[line])
	}
	# Conditional without a block
	else if (buf[line] !~ /^[ \t]*$/) {
		condition = buf[line]
		buf[line] = "void filter" NR "() {print}"
	}

	# Inject the condition into the documentation
	if (condition != "") {
		buf[line + 2] = buf[line]
		buf[line + 1] = buf[line - 1]
		buf[line - 1] = "@pre"
		buf[line] = "	\\verbatim " condition " \\endverbatim"
		line += 2
	}
}

##
# Strip the indenting from the documentation.
#
# This benefits verbatim and code formatting.
#
doc && /^[ \t]*#/ {
	sub(/^[ \t]*# ?/, "", buf[line])
}

##
# Collect function parameters.
#
doc && buf[line] ~ /[\\@]param(\[(in|out|in,out)\])?[ \t]/ {
	i = -1
	while (fargs[++i]);
	fargs[i] = buf[line]
	sub(/[ \t]*[\\@]param(\[(in|out|in,out)\])?[ \t]*/, "", fargs[i])
	sub(/[ \t].*/, "", fargs[i])
}

##
# Detect that the function returns something.
#
doc && buf[line] ~ /[\\@](return|retval)/ {
	fret = 1
}

##
# Replace static regular expressions with strings.
#
!doc && buf[line] ~ /^(("(\\.|[^"])*")?[^"\/])*\/(\\.|[^\/])+\// {
	while (match(buf[line], /^(("(\\.|[^"])*")?[^"\/])*\/(\\.|[^\/])+\//)) {
		debug("replace regex in line " NR " buffer " line)
		behind = RSTART + RLENGTH
		regex = substr(buf[line], RSTART, RLENGTH)
		match(regex, /^(("(\\.|[^"])*")?[^"\/])*/)
		regex = substr(buf[line], RLENGTH + 2, length(regex) - RLENGTH - 2)
		gsub(/"/, "\\\"", regex)
		gsub(/\\\//, "/", regex)
		buf[line] = substr(buf[line], 1, RLENGTH) "\"" regex "\"" substr(buf[line], behind)
	}
}

##
# Assume the current line does not contain a comment.
#
{
	comment = 0
}

##
# Make C comments out of awk comments.
#
!doc && buf[line] ~ /^(("(\\.|[^"])*")?[^"#])*#/ {
	comment = 1
	debug("comment in line " NR " buffer " line)
	match(buf[line], /^(("(\\.|[^"])*")?[^"#])*#/)
	# Add semicolon in front of comment
	nocomment = substr(buf[line], RSTART, RLENGTH - 1)
	if (nocomment ~ /[^{}; \t][ \t]*$/) {
		sub(/[ \t]*$/, ";&", nocomment)
	}
	buf[line] = nocomment "/*" substr(buf[line], RSTART + RLENGTH) " */"
}

##
# Replace $n with incol[n].
#
!doc && buf[line] ~ /^(("(\\.|[^"])*")?[^"$])*\$[0-9]+/ {
	while (match(buf[line], /^(("(\\.|[^"])*")?[^"$])*\$[0-9]+/)) {
		code = substr(buf[line], RSTART, RLENGTH)
		incol = code
		sub(/\$[0-9]+$/, "", code)
		sub(/.*\$/, "", incol)
		buf[line] = code "incol[" incol "]" substr(buf[line], RSTART + RLENGTH)
	}
}

##
# Append a semicolon.
#
!doc && !comment && buf[line] ~ /[^{};\\]$/ {
	buf[line] = buf[line] ";"
}

