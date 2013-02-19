#!/usr/bin/awk -f

# Seperates C instructions into individual lines, streamlining the formatting

BEGIN {
	RS = "\0"
	# Collect parameters
	for (i = 1; i < ARGC; i++) {
		if (ARGV[i] ~ /^-.$/) {
			args = args " " ARGV[i] " " ARGV[i + 1] " "
			delete ARGV[i++]
			delete ARGV[i]
		} else if (ARGV[i] ~ /^-/) {
			args = args " " ARGV[i] " "
			delete ARGV[i]
		}
	}
}

#
# Accumulate and preprocess files so they become easier to parse
#
{
	# Preprocess file, so there is no trouble with unknown symbols
	cmd = (ENVIRON["CPP"] ? ENVIRON["CPP"] : "cpp") " " args incdirs FILENAME " 2> /dev/null"
	$0 = ""
	while (cmd | getline line) {
		$0 = $0 line "\n"
	}
	close(cmd)
	# Remove comments
	gsub(/\/\*(\*[^\/]|[^\*])*\*\//, "")
	# Isolate preprocessor comments
	gsub("(^|\n)[[:space:]]*#[^\n]*", "&\177")
	# Remove escaped newlines
	gsub(/\\[\r\n]+/, "")
	# Collapse spaces
	gsub(/[[:space:]\r\n]+/, " ")
	# Remove obsolete spaces
	split("{}()|?*+^[].", ctrlchars, "")
	for (i in ctrlchars) {
		gsub(" \\" ctrlchars[i], ctrlchars[i])
		gsub("\\" ctrlchars[i] " ", ctrlchars[i])
	}
	split("#=!<>;:,/-%~\"\177", ctrlchars, "")
	for (i in ctrlchars) {
		gsub(" " ctrlchars[i], ctrlchars[i])
		gsub(ctrlchars[i] " ", ctrlchars[i])
	}
	gsub(" \\&", "\\&")
	gsub("\\& ", "\\&")
	# Add newlines behind statements
	gsub(/;/, "&\n")
	gsub(/\177/, "\n")
	# Segregate nested code
	gsub(/\{/, "\n{\n")
	gsub(/\}/, "\n}\n")
	# Segregate labels from the code following them
	gsub(/(^|\n)[[:alnum:]_:]+:/, "&\n")
	gsub(/(^|\n)case [^\n]+:/, "&\n")
	# Remove empty lines
	gsub(/\n+/, "\n")
	printf "#filename %s\n%s", FILENAME, $0
}

