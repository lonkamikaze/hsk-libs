#!/usr/bin/awk -f
#
# This script provides a small command line XML editor.
#
# It parses the subset of XML used by ARM Keil µVision configuration files
# and provides arguments to navigate, search, edit and print the parsed
# XML tree.
#
# Every command applies to the current selection. The current selection
# may refer to several nodes in the tree.
#
# The command syntax is:
#	"-" command [ ":" argument ]
#
# The following command arguments are supported:
# - select:         Selects a path using a filter argument, see cmdSelect()
# - search:         Selects all the subtrees matching the given filter
#                   argument, see cmdSearch()
# - set:            Sets the data of a node, see cmdSet()
# - attrib:         Sets a named attribute, see cmdAttrib()
# - insert:         Inserts a new child node into the selected nodes, see
#                   cmdInsert()
# - selectInserted: Select all nodes created during the last insert operation,
#                   see cmdSelectInserted()
# - delete:         Removes the selected nodes from the tree, see cmdDelete()
# - print:          Print the children and data of the selected nodes, see
#                   cmdPrint()
#

#
# Parse arguments and initialise globals.
#
BEGIN {
	FS = ""
	RS = ""
	SUBSEP = ","
	INDENT = "  "
	ROOT = "-1,-1"
	depth = 0
	count[-1] =  0
	count[0] = 0
	selection[ROOT] = 1
	uVisionsugar = 1
	for (i = 1; i < ARGC; i++) {
		if (ARGV[i] ~ /^[ \t\n]*-/) {
			p = 1 * p
			commands[p] = ARGV[i]
			sub(/[ \t\n]*-/, "", commands[p])
			sub(/:.*/, "", commands[p])
			arguments[p] = ARGV[i]
			sub(/[^:]*(:|$)/, "", arguments[p++])
			delete ARGV[i]
		}
	}
}

#
# Return whether an array is empty.
#
# @param array
#	The array to check
# @retval 1
#	The array is empty
# @retval 0
#	The array contains at least one element
#
function empty(array,
i) {
	for (i in array) {
		return 0
	}
	return 1
}

#
# Split a string containing attributes into a string array with
# "attribute=value" entries.
#
# @param str
#	The string to split into attributes
# @param results
#	The array to store the results in
# @return
#	The count of attributes
#
function explode(str, results,
i, array, quoted, count, len) {
	len = split(str, array)
	str = ""
	for (i = 1; i <= len; i++) {
		if (array[i] == "\"") {
			quoted = !quoted
			continue
		}

		if (!quoted && array[i] ~ /[ \t\n]/) {
			if (length(str)) {
				results[count++] = str
				str = ""
			}
			continue
		}

		if (array[i] == "\\") {
			i++
		}

		str = str array[i]
	}
	if (length(str)) {
		results[count++] = str
	}
	return count
}

#
# Escapes quotation marks and backslashes with backslashes.
#
# @param str
#	The string to escape
# @return
#	The escaped string
#
function escape(str) {
	gsub(/\\/, "\\\\", str)
	gsub(/"/, "\\\"", str)
	return str
}

#
# This function lets you define a selection.
#
# A selection filter is a series of node defintions divided by /. Identifiers
# may contain glob patterns.
#
# A / at the beginning of the filter selects the root node, which contains
# the root nodes of all XML trees parsed. Other wise the filter is relative
# to the current selections.
#
# The node ./ refers to the current node and ../ to the parent node.
# This can be used to move through the tree relative to the current selection
# or to select the parent of a node that matches a filtering condition.
#
# A node selection has the following syntax:
#	node = tag [ "[" attributes "]" ] [ "=" value ]
#
# Attributes have the following syntax:
#	attributes = attribute "=" ( value | '"' value '"' ) [ " " attributes ]
#
# Values are strings or glob patterns.
#
# @param str
#	The selection filter
#
function cmdSelect(str,
ident, node, ns, i, attrib, attribs, value) {
	gsub(/\/+/, "/", str)

	while (length(str)) {
		ident = str
		sub(/\/.*/, "/", ident)
		sub(/[^\/]*\/?/, "", str)

		# Select /
		if (ident == "/") {
			delete selection
			selection[ROOT] = 1
		}
		# Select ./
		else if (ident ~ /^\.\/?$/) {
			# Nothing to do here
		}
		# Select ../
		else if (ident ~ /^\.\.\/?$/) {
			delete ns
			for (node in selection) {
				ns[parent[node]]
			}
			delete selection
			for (node in ns) {
				selection[node] = 1
			}
		}
		# Select tags
		else {
			sub(/\/$/, "", ident)
			gsub(/\./, "\\.", ident)
			gsub(/\*/, ".*", ident)
			gsub(/\?/, ".", ident)

			attrib = ident
			sub(/[\[=].*/, "", ident)
			delete ns
			for (node in selection) {
				for (i = 0; children[node, i]; i++) {
					if (tags[children[node, i]] ~ "^" ident "$") {
						ns[children[node, i]]
					}
				}
			}
			delete selection
			for (node in ns) {
				selection[node] = 1
			}

			# Recover ident
			ident = attrib

			# Select tags with an attribute
			if (ident ~ /\[.*=.*\]/) {
				attrib = ident
				sub(/[^[]*\[/, "", attrib)
				sub(/\]($|=)/, "", attrib)
				explode(attrib, attribs)
				if (ident ~ /\]=/) {
					sub(/.*\]=/, "=" , ident)
				} else {
					ident = ""
				}
			}

			# Check the detected attributes
			for (attrib in attribs) {
				value = attribs[attrib]
				attrib = value
				sub(/=.*/, "", attrib)
				sub(/[^=]*=/, "", value)

				for (node in selection) {
					delete selection[node]
					for (i = 0; attributeNames[node, i]; i++) {
						if ((attributeNames[node, i] ~ "^" attrib "$") && (attributeValues[node, i] ~ "^" value "$")) {
							selection[node] = 1
						}
					}
				}
			}

			# Select tags with a given value
			sub(/[^=]*/, "", ident)
			if (ident ~ /^=/) {
				sub(/^=/, "", ident)
				
				for (node in selection) {
					if (contents[node] !~ "^" ident "$") {
						delete selection[node]
					}
				}
			}
		}
	}
	
}

#
# This function selects any subtree of the current selection that matches
# the given selection filter.
#
# The filter syntax is identical with that of cmdSelect().
#
# @param str
#	The selection filter
#
function cmdSearch(str,
node, i, lastSelection, matches) {
	while (!empty(selection)) {
		for (node in selection) {
			lastSelection[node]
		}

		cmdSelect(str)

		for (node in selection) {
			matches[node]
		}
		delete selection

		for (node in lastSelection) {
			for (i = 0; children[node, i]; i++) {
				selection[children[node, i]] = 1
			}
		}
		delete lastSelection
	}
	for (node in matches) {
		selection[node] = 1
	}
}

#
# Changes the content of a node.
#
# This does not affect subnodes.
#
# @param value
#	The value to set the node content to
#
function cmdSet(value,
node) {
	for (node in selection) {
		contents[node] = value
	}
}

#
# Changes an attribute of a node.
#
# It accepts a singe string in the shape:
#	attribute "=" value
#
# @param str
#	A single attribute definition
#
function cmdAttrib(str,
node, i, name) {
	name = str
	sub(/=.*/, "", name)
	sub(/[^=]*=/, "", str)
	for (node in selection) {
		# Seek the attribute
		for (i = 0; attributeNames[node, i] && attributeNames[node, i] != name; i++);
		# The clever part is, that it either points behind the other
		# attributes or to the attribute that has to be updated, either
		# way it can just be overwritten.
		attributeNames[node, i] = name
		attributeValues[node, i] = str
	}
}

#
# Inserts new nodes into all selected nodes, uses the same syntax as
# cmdSelect() does.
#
# @param str
#	A node definition like the ones used for selection filters
#
function cmdInsert(str,
node, name, attributes, attribStr, value, count, i, insert) {
	name = str
	sub(/[\[=].*/, "", name)
	sub(/^[^\[=]*/, "", str)
	if (str ~ /^\[.*\]/) {
		attribStr = str
		sub(/\[/, "", attribStr)
		sub(/\]($|=)/, "", attribStr)
		count = explode(attribStr, attributes)
		if (str ~ /\]=/) {
			sub(/.*\]=/, "=", str)
		} else {
			str = ""
		}
	}
	value = str
	sub(/^=/, "", value)

	# Free the list of recently inserted nodes
	delete inserted

	# The new nodes simply are numbered instead of having structured
	# IDs. Because the node can only have one parent it needs to be
	# created for each selection to insert.
	for (node in selection) {
		insert = "NEW" newNodes++
		# Remember which nodes were inserted into the tree
		inserted[insert]
		# Set name and value
		tags[insert] = name
		contents[insert] = value
		# Set attributes
		for (i = 0; i < count; i++) {
			attributeNames[insert, i] = attributes[i]
			sub(/=.*/, "", attributeNames[insert, i])
			attributeValues[insert, i] = attributes[i]
			sub(/[^=]*=/, "", attributeValues[insert, i])
		}
		# Record parent
		parent[insert] = node
		# Hook into tree (i.e. add as a child to the current selection
		for (i = 0; children[node, i]; i++);
		children[node, i] = insert
	}
}

#
# Select the nodes created during the last insert operation.
#
function cmdSelectInserted(dummy,
node) {
	delete selection
	for (node in inserted) {
		selection[node]
	}
}

#
# Unhooks a selected node from the tree, it's still there and can be navigated
# out of by selecting "..".
#
function cmdDelete(dummy,
node, i) {
	# Delete the selected nodes
	for (node in selection) {
		for (i = 0; children[parent[node], i] != node; i++);
		for (; children[parent[node], i + 1]; i++) {
			children[parent[node], i] = children[parent[node], i + 1]
		}
		delete children[parent[node], i]
	}
}

#
# Print the current selection.
#
function cmdPrint(dummy,
node) {
	for (node in selection) {
		printNode(0, node)
	}
}

#
# Prints children and contents of the given node
#
# @param indent
#	The indention depth of the current node
# @param node
#	The node to print
#
function printNode(indent, node,
prefix, i, p) {
	# Create indention string
	for (i = 0; i < indent; i++) {
		prefix = INDENT prefix
	}
	# Print all children and the node
	for (i = 0; children[node, i]; i++) {
		# µVision separates tags on this level by a newline
		if (uVisionsugar && parent[node] == "-1" SUBSEP "-1") {
			printf "\n"
		}
		# Indent and opening tag
		printf "%s<%s", prefix, tags[children[node, i]]
		# Attributes
		for (p = 0; attributeNames[children[node, i], p]; p++) {
			printf " %s=\"%s\"", attributeNames[children[node, i], p], escape(attributeValues[children[node, i], p])
		}
		# Current child has children
		if (children[children[node, i], 0]) {
			# Close openining tag
			printf ">\n"
			# Recursively print children of this child
			printNode(indent + 1, children[node, i])
			# Close tag
			printf "%s</%s>\n", prefix, tags[children[node, i]]
		}
		# Current child only has content
		else if (contents[children[node, i]]) {
			# Close opening tag, print content and add closing
			# tag all in the same line
			printf ">%s</%s>\n", contents[children[node, i]], tags[children[node, i]]
		}
		# Current child ends with starts with <?
		else if (tags[children[node, i]] ~ /^\?/) {
			printf " ?>\n"
		}
		# Current child is empty
		else {
			# Not so compact style, used by µVision
			if (uVisionsugar) {
				printf "></%s>\n", tags[children[node, i]]
			}
			# Compact style
			else {
				printf " />\n"
			}
		}
	}
	# µVision separates tags on this level by a newline
	if (uVisionsugar && parent[node] == "-1" SUBSEP "-1") {
		printf "\n"
	}
	# Print contents of the node
	if (contents[node]) {
		printf "%s%s\n", prefix, contents[node]
	}
}

#
# Parse the XML tree
#
# Abbreviations:
# - d = depth
# - c = count
#
# Properties:
# - tags [d, c]
# - contents [d, c]
# - attributeNames [d, c, i]
# - attributeValues [d, c, i]
# - children [d, c, i]
# - parent [d, c]
#
{
	while (/<.*>/) {
		# Get the current content
		content = $0
		sub(/[ \t\n]*<.*/, "", content)
		
		# Get the next tag
		sub(/[^<]*</, "")
		tag = $0
		sub(/>.*/, "", tag)
		sub(/[^>]*>/, "")

		# This is a closing tag
		if (tag ~ /^\//) {
			depth--
			sub(/^[ \t\n]*/, "", content)
			sub(/[ \t\n]*$/, "", content)
			contents[depth, count[depth] - 1] = content
		}

		# This is an opening tag
		else {
			# Register with the parent
			current = depth SUBSEP count[depth]
			parent[current] = (depth - 1) SUBSEP (count[depth - 1] - 1)
			for (i = 0; children[parent[current], i]; i++);
			children[parent[current], i] = depth SUBSEP count[depth]

			# Raw tag
			tagName = tag

			# Name of the tag
			sub(/[ \t\n].*/, "", tagName)
			sub(/(\/|\?)$/, "", tagName)
			tags[current] = tagName

			# Get attributes
			tagAttributes = tag
			sub(/[^ \t\n]*[ \t\n]*/, "", tagAttributes)
			sub(/(\/|\?)$/, "", tagAttributes)
			countAttributes = explode(tagAttributes, tagAttributesArray)
			for (i = 0; i < countAttributes; i++) {
				attributeName = tagAttributesArray[i]
				sub(/=.*/, "", attributeName)
				attributeValue = tagAttributesArray[i]
				sub(/[^=]*=/, "", attributeValue)
				#attributes[current, attributeName] = attributeValue
				attributeNames[current, i] = attributeName
				attributeValues[current, i] = attributeValue
			}

			count[depth]++
			# This is not a self-closing tag
			if (tag !~ /^\?.*\?$/ && tag !~ /\/$/) {
				depth++
				count[depth] += 0
			}
		}
	}
}

#
# Execute the specified commands.
#
END {
	for (i = 0; commands[i]; i++) {
		if (commands[i] == "select") {
			cmdSelect(arguments[i])
		} else if (commands[i] == "search") {
			cmdSearch(arguments[i])
		} else if (commands[i] == "set") {
			cmdSet(arguments[i])
		} else if (commands[i] == "attrib") {
			cmdAttrib(arguments[i])
		} else if (commands[i] == "insert") {
			cmdInsert(arguments[i])
		} else if (commands[i] == "selectInserted") {
			cmdSelectInserted(arguments[i])
		} else if (commands[i] == "delete") {
			cmdDelete(arguments[i])
		} else if (commands[i] == "print") {
			cmdPrint(arguments[i])
		}
	}
}

