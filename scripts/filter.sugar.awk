#!/usr/bin/awk -f
#
# Filter certain syntactical sugar from C code.
#

##
# Remove indented preprocessor instructions, they are usually just in place
# hacks that don't need to show up in the docs.
#
/^[ \t]+#/ {sub(/.*/, "")}

##
# Detect the beginning of a documentation block.
#
/\/\*\*/ {comment=1}

##
# Detect the end of a documentation block.
#
/\*\// {comment=0}

##
# Align documentation so verbatim and code sections are formatted
# correctly.
#
comment {sub(/^[ \t]*\* ?/, "")}

##
# Print the updated line.
#
1

