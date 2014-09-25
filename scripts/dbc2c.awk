#!/usr/bin/awk -f
#
# This script parses Vector CAN DBs (.dbc files), such as can be created
# using Vector CANdb++.
#
# A subset of the parsed information is output using a set of templates.
#
# @note
#	Pipe the input through "iconv -f CP1252" so GNU AWK doesn't choke
#	on non-UTF-8 characters in comments.
# @warning
#	Templates are subject to change, which may break the output for
#	your use case. To prevent this retain your own copy of the
#	templates directory and set the \ref dbc2c_env_TEMPLTES variable.
#	Old templates will continue working, though they might cause
#	deprecation warnings.
#
# \section dbc2c_env Environment
#
# The script uses certain environment variables.
#
# \subsection dbc2c_env_DEBUG DEBUG
#
# | Value               | Effect
# |---------------------|-----------------------------------------
# | 0, ""               | Debugging output is deactivated
# | 1, any string != "" | Debugging output to stderr is activated
# | > 1                 | Additionally any string read is output
#
# \subsection dbc2c_env_TEMPLTES TEMPLATES
#
# This variable can be used to pass the template directory to the script.
#
# If the \c LIBPROJDIR environment variable is set it defaults to
# <tt>${LIBPROJDIR}/scripts/templates.dbc2c</tt>, otherwise it defaults
# to the relative path <tt>scripts/templates.dbc2c</tt>.
#
# \subsection dbc2c_env_DATE DATE
#
# This can be used to define the date string provided to <tt>header.tpl</tt>.
#
# It defaults to the output of the \c date command.
#
# \section dbc2c_vts Value Tables
#
# Since values in value tables only consist of a number and description,
# the first word of this description is used as a symbolic name for a given
# value.
#
# All non-alhpanumeric characters of this first word will be converted to
# underscores. Redundancies will be resolved by appending the value to the
# word that signifies the name.
#
# This functionality is implemented in the function getUniqueEnum().
#
# \section dbc2c_templates Templates
#
# This section describes the templates that are used by the script
# and the arguments passed to them. Templates are listed in the
# chronological order of use.
#
# \subsection dbc2c_templates_attributes Special Attributes
#
# Some of the arguments provided depend on custom attributes:
# | Template    | Argument | Attribute           | Object
# |-------------|----------|---------------------|--------------------------
# | sig.tpl     | start    | GenSigStartValue    | Signal
# | msg.tpl     | fast     | GenMsgCycleTimeFast | Message
# | msg.tpl     | cycle    | GenMsgCycleTime     | Message
# | msg.tpl     | delay    | GenMsgDelayTime     | Message
# | msg.tpl     | send     | GenMsgSendType      | Message
# | timeout.tpl | timeout  | GenSigTimeoutTime   | Relation (ECU to Signal)
#
# These and more attributes are specified by the
# <b>Vector Interaction Layer</b>.
#
# \subsection dbc2c_templates_data Inserting Data
#
# Templates are arbitrary text files that are provided with a set of
# arguments. Arguments have a symbolic name through which they can be used.
# In the following sections they are called fields, because they are provided
# to the template() function in an associative array.
#
# Inserting data into a template is simple:
#
#	<:name:>
#
# The previous example adds the data in the field \c name into the file.
# It can be surrounded by additional context:
#
#	#define <:name:> <:value:>
#
# If \c name is "FOO_BAR" and \c value is 1337, this line would be resolved
# to:
#
#	#define FOO_BAR 1337
#
# It may be desired to reformat some of those values. A number of special
# filters (see filter()) as well es printf(3) style formatting is available.
# E.g. \c name can be converted to camel case and \c value to hex:
#
#	#define <:name:camel:%-16s:> <:value:%#x:>
#
# The output would look like this:
#
#	#define fooBar           0x539
#
# An important property of templates is that arguments may contain multiple
# lines. In that case the surrounding text is preserved for every line, which
# is useful to format multiline text or lists. This can be used to create
# lists or provide visual sugar around text:
#
#	+----[ <:title:%-50s:> ]----+
#	| <:text:%-60s:> |
#	+--------------------------------------------------------------+
#
# Output could look like this:
#
#	+----[ Racecar by Matt Brown                              ]----+
#	| 'Racecar - Searching for the Limit in Formula SAE'           |
#	| is available for download from:                              |
#	|     http://www.superfastmatt.com/2011/11/book.html           |
#	+--------------------------------------------------------------+
#
# Multi line data is treated as an array of individual lines. Besides
# descriptions in DBC files multiline data can also originate from lists
# provided by this script in order to allow describing the relations
# between ECUs, messages, signals etc..
#
# In some cases it is prudent to print lines conditionally. For that
# conditionals are provided:
#
#	<?name?>
#
# If the reverenced field evaluates to \c true, the conditional is removed
# from the line. If it evaluates to \c false, the entire template line is
# omitted.
#
# \subsection dbc2c_templates_header header.tpl
#
# Used once with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | date     | string   |  The current date
# | db       | string[] |  A list of identifiers for the parsed DBCs
#
# \subsection dbc2c_templates_file file.tpl
#
# Used for each input file with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | db       | string   | An identifier for this input file
# | file     | string   | The file name
# | comment  | string[] | The comment text for this CANdb
# | ecu      | string[] | A list of ECUs provided with this file
#
# \subsection dbc2c_templates_sigid sigid.tpl
#
# This template should only contain a single line that produces a unique
# identifier string for a signal, using the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | msg      | int      | The message ID
# | msgname  | string   | The message name
# | sig      | string   | The signal name
#
# Signal names are not globally unique, thus an identifier must contain
# a message reference to avoid name collisions.
#
# \subsection dbc2c_templates_ecu ecu.tpl
#
# Used for each ECU with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | ecu      | string   | An identifier for the ECU
# | comment  | string[] | The comment text for this ECU
# | db       | string   | The input file identifier
# | txid     | int[]    | A list of message IDs belonging to messages sent by this ECU
# | txname   | string[] | A list of message names sent by this ECU
# | rx       | string[] | A list of signals received by this ECU
# | rxid     | string[] | A list of unique signal identifiers received by this ECU
#
# \subsection dbc2c_templates_msg msg.tpl
#
# Used for each message with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | msg      | int      | The message ID
# | name     | string   | The message name
# | comment  | string[] | The comment text for this message
# | sig      | string[] | A list of signal names contained in this message
# | sigid    | string[] | A list of signal identifiers contained in this message
# | ecu      | string   | The ECU sending this message
# | ext      | bool     | Message ID is extended
# | dlc      | int      | The data length count
# | cycle    | int      | The cycle time of this message
# | fast     | int      | The fast cycle time of this message
# | delay    | int      | The minimum delay time between two transmissions
# | send     | string   | The send type (cyclic, spontaneous etc.)
# | sgid     | string[] | A list of signal group ids
# | sgname   | string[] | A list of signal group names
#
# \subsection dbc2c_templates_siggrp siggrp.tpl
#
# Used for each signal group with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | id       | string   | The ID of the signal group (created using sigid.tpl)
# | name     | string   | The name of the signal group
# | msg      | int      | The ID of the message containing this signal group
# | msgname  | string   | The name of the message containing this signal group
# | sig      | string[] | A list of signals belonging to this signal group
# | sigid    | string[] | A list of signal identifers belonging to this signal group
#
# \subsection dbc2c_templates_sig sig.tpl
#
# Used for each signal with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | name     | string   | The signal name
# | id       | string   | The unique signal identifier created with sigid.tpl
# | comment  | string[] | The comment text for this signal
# | enum     | bool     | Indicates whether this signal has a value table
# | msg      | int      | The ID of the message sending this signal
# | sgid     | string[] | The signal groups containing this signal
# | sgname   | string[] | The names of the signal groups containing this signal
# | ecu      | string[] | A list of the ECUs receiving this signal
# | intel    | bool     | Intel (little endian) style signal
# | motorola | bool     | Motorola (big endian) style signal
# | signed   | bool     | The signal is signed
# | sbit     | int      | The start bit (meaning depends on endianess)
# | len      | int      | The signal length
# | start    | int      | The initial (default) signal value (raw)
# | calc16   | string[] | A rational conversion function (see \ref dbc2c_templates_sig_calc16)
# | min      | int      | The raw minimum value
# | max      | int      | The raw maximum value
# | off      | int      | The raw offset value
# | getbuf   | string[] | The output of <tt>sig_getbuf.tpl</tt>
# | setbuf   | string[] | The output of <tt>sig_setbuf.tpl</tt>
#
# \subsubsection dbc2c_templates_sig_calc16 calc16
#
# A rational conversion function for the raw signal value \c x
# and formatting factor \c fmt into a real value as defined
# by the linear factor and offset in the DBC, this function
# uses up to 16bit integers.
#
# \subsubsection dbc2c_templates_sig_buf sig_getbuf.tpl, sig_setbuf.tpl
#
# These templates can be used to construct static byte wise signal getters
# and setters.
#
# For signed signals <tt>sig_getbuf.tpl</tt> is first called with the
# following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | sign     | string   | "-"
# | byte     | int      | The byte containing the most significant bit
# | align    | int      | The position of the most significant bit in the byte
# | msk      | int      | 1
# | pos      | int      | The position in front of the entire read signal
# | int8     | bool     | Indicates whether an 8 bit integer suffices to contain the signal
# | int16    | bool     | Indicates whether a 16 bit integer suffices to contain the signal
# | int32    | bool     | Indicates whether a 32 bit integer suffices to contain the signal
#
# These arguments can be used to duplicate the signed bit and shift it
# in front.
#
# Both templates are used for each touched signal byte with the following
# arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | sign     | string   | "+"
# | byte     | int      | The signal byte
# | align    | int      | The least significant bit within the byte belonging to the signal
# | msk      | int      | A bit mask to mask the aligned signal bits
# | pos      | int      | The position to shift the resulting bits to
# | int8     | bool     | Indicates whether an 8 bit integer suffices to address the desired bit
# | int16    | bool     | Indicates whether a 16 bit integer suffices to address the desired bit
# | int32    | bool     | Indicates whether a 32 bit integer suffices to address the desired bit
#
# \subsubsection dbc2c_templates_sig_enum sig_enum.tpl, sig_enumval.tpl
#
# In case a value table is assigned to the signal, <tt>sig_enum.tpl</tt> is
# called with all the arguments provided to <tt>sig.tpl</tt>.
#
# For each entry in the value table <tt>sig_enumval.tpl</tt> is called
# with these additional arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | enumval  | int      | The value
# | enumname | string   | The name of the value
# | comment  | string[] | The comment part of the value description
#
# \subsection dbc2c_templates_timeout timeout.tpl
#
# Used for each timeout with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | ecu      | string   | The ECU that times out
# | sig      | string   | The signal that is expected by the ECU
# | sigid    | string   | The unique identifier for the expected signal
# | timeout  | int      | The timeout time
# | msg      | int      | The ID of the CAN message containing the signal
# | msgname  | string   | The name of the CAN message containing the signal
#
# \subsection dbc2c_templates_enum enum.tpl
#
# Invoked for every value table with the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | enum     | string   | The name of the value table
# | db       | string   | The name of the CAN DB this enum was defined in
#
# \subsubsection dbc2c_templates_enumval enumval.tpl
#
# Invoked for every value defined in a value table. All the template arguments
# for \c enum.tpl are available in addition to the following arguments:
# | Field    | Type     | Description
# |----------|----------|-------------
# | val      | int      | The value
# | name     | string   | The symbolic name for the value
# | comment  | string[] | The comment part of the value description
#

##
# Initialises globals.
#
BEGIN {
	# Environment variables
	DEBUG = (DEBUG ? DEBUG : ENVIRON["DEBUG"])
	TEMPLATES = (TEMPLATES ? TEMPLATES : ENVIRON["TEMPLATES"])
	DATE = (DATE ? DATE : ENVIRON["DATE"])

	# Template directory
	if (!TEMPLATES) {
		path = ENVIRON["LIBPROJDIR"]
		sub(/.+/, "&/", path)
		TEMPLATES = path "scripts/templates.dbc2c"
	}
	sub(/\/?$/, "/", TEMPLATES)

	# Generating date
	if (!DATE) {
		"date" | getline DATE
		close("date")
	}

	FILENAME = "/dev/stdin"

	# Regexes for different types of data
	rLF = "\n"
	rFLOAT = "-?[0-9]+(\\.[0-9]+)?([eE][-+][0-9]+)?"
	rINT = "-?[0-9]+"
	rID = "[0-9]+"
	rDLC = "[0-9]+"
	rSEP = "[:;,]"
	rSYM = "[a-zA-Z0-9_]+"
	rSYMS = "(" rSYM ",)*" rSYM
	rSIG = "[0-9]+\\|[0-9]+@[0-9]+[-+]"
	rVEC = "\\(" rFLOAT "," rFLOAT "\\)"
	rBND = "\\[" rFLOAT "\\|" rFLOAT "\\]"
	rSTR = "\"([^\"]|\\\\.)*\"" # CANdb++ does not support escaping
	                            # characters like ", however it parses
	                            # them just fine.

	# Type strings
	tDISCARD["BS_"]
	t["SIG_ENUM"] = "VAL_"
	t["ENUM"] = "VAL_TABLE_"
	t["SYMBOLS"] = "NS_"
	t["VER"] = "VERSION"
	t["ECU"] = "BU_"
	t["MSG"] = "BO_"
	t["SIG"] = "SG_"
	t["ENV"] = "EV_"
	t["COM"] = "CM_"
	t["ATTRDEFAULT"] = "BA_DEF_DEF_"
	t["RELATTRDEFAULT"] = "BA_DEF_DEF_REL_"
	t["ATTRRANGE"] = "BA_DEF_"
	t["RELATTRRANGE"] = "BA_DEF_REL_"
	t["ATTR"] = "BA_"
	t["RELATTR"] = "BA_REL_"
	t["EDLC"] = "ENVVAR_DATA_"
	t["TX"] = "BO_TX_BU_"
	t["SIG_GRP"] = "SIG_GROUP_"

	# Relationship symbols
	t["ECU_SIG"] = "BU_SG_REL_"
	t["ECU_ENV"] = "BU_EV_REL_"
	t["ECU_MSG"] = "BU_BO_REL_"

	# Not implemented!
	# This is marked by the symbol being indexed with its literal name.
	t["NS_DESC_"] = "NS_DESC_"
	t["CAT_DEF_"] = "CAT_DEF_"
	t["CAT_"] = "CAT_"
	t["FILTER"] = "FILTER"
	t["EV_DATA_"] = "EV_DATA_"
	t["SGTYPE_"] = "SGTYPE_"
	t["SGTYPE_VAL_"] = "SGTYPE_VAL_"
	t["BA_DEF_SGTYPE_"] = "BA_DEF_SGTYPE_"
	t["BA_SGTYPE_"] = "BA_SGTYPE_"
	t["SIG_TYPE_REF_"] = "SIG_TYPE_REF_"
	t["SIG_VALTYPE_"] = "SIG_VALTYPE_"
	t["SIGTYPE_VALTYPE_"] = "SIGTYPE_VALTYPE_"
	t["SG_MUL_VAL_"] = "SG_MUL_VAL_"

	# Attribute types
	atSTR = "STRING"
	atENUM = "ENUM"
	atINT = "INT";     atNUM[atINT]
	atFLOAT = "FLOAT"; atNUM[atFLOAT]
	atHEX = "HEX";     atNUM[atHEX]

	# Environment variable types
	etINT = "INT"
	etFLOAT = "FLOAT"
	etDATA = "DATA"
	eTYPE[0] = etINT
	eTYPE[1] = etFLOAT

	# Prominent attributes
	aSTART = "GenSigStartValue"
	aFCYCLE = "GenMsgCycleTimeFast"
	aCYCLE = "GenMsgCycleTime"
	aDELAY = "GenMsgDelayTime"
	aSEND = "GenMsgSendType"
	aTIMEOUT = "GenSigTimeoutTime"

	# Global error indicator
	errno = 0
}

##
# Strip DOS line endings and make sure there is a new line symbol at the
# end of the line, so multiline definitions can be parsed.
#
{
	gsub(/[\r\n]*$/, "\n")
}

##
# Prints an error message on stderr and exits.
#
# @param no
#	The number to set errno to
# @param msg
#	The error message
#
function error(no, msg) {
	errno = no
	print "dbc2c.awk: ERROR: " FILENAME "(" NR "): " msg > "/dev/stderr"
	exit
}

##
# Prints a warning message on stderr.
#
# @param msg
#	The message to print
#
function warn(msg) {
	print "dbc2c.awk: WARNING: " msg > "/dev/stderr"
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
		print "dbc2c.awk: " msg > "/dev/stderr"
	}
}

##
# Makes sure $0 is not empty.
#
function buffer() {
	sub(/^[	 ]*/, "")
	if (!$0) {
		getline
		gsub(/[\r\n]*$/, "\n")
		sub(/^[	 ]*/, "")
	}
}

##
# Special function to fetch a string from the buffer.
#
# This is a special case, because strings may span multiple lines.
# This function supports strings with up to 256 lines.
#
# @return
#	The fetched string
#
function fetchStr(dummy,
	str,i) {
	buffer()
	if ($0 !~ /^"/) {
		return ""
	}
	# Assume strings are no longer than 256 lines
	while ($0 !~ "^(" rSTR ")" && i++ < 256) {
		getline str
		gsub(/[\r\n]*$/, "\n", str)
		$0 = $0 str
	}
	return strip(fetch(rSTR))
}

##
# Fetch the next token from the input buffer, matching a given type.
#
# @param types
#	A regular expression describing the type of data to be fetched
# @return
#	The fetched string of data
#
function fetch(types,
	str, re) {
	buffer()
	if (match($0, "^(" types ")")) {
		str = substr($0, RSTART, RLENGTH)
		# Cut str from the beginning of $0
		$0 = substr($0, RSTART + RLENGTH)
	}
	if (DEBUG > 1 && str !~ /^[ \t\n]*$/) {
		debug("fetch: " str)
	}
	if (str) {
		fetch_loop_detect = 0
	} else if (fetch_loop_detect++ >= 100) {
		error(11, "infinite loop detected!")
	}
	return str
}

##
# Returns the expresion with ^ and $ at beginning and end to make ~ match
# entire strings only.
#
# @param re
#	The expression to wrap
# @return
#	An expression for matching entire strings
#
function whole(re) {
	return "^(" re ")$"
}

##
# Remove quotes and escapes from strings.
#
# This function is used by fetchStr().
#
# @param str
#	The string to unescape
# @return
#	The litreal string
#
function strip(str) {
	sub(/^"/, "", str)
	sub(/"$/, "", str)
	# Unescape "
	gsub(/\\"/, "\"", str)
	return str
}

##
# Returns the context type for a string.
#
# @param str
#	The string to interpret
# @retval "sig"
#	The context is a signal
# @retval "msg"
#	The context is a message
# @retval "ecu"
#	The context is an ECU
# @retval "env"
#	The context is an environment variable
# @retval "db"
#	The context is the DB
#
function getContext(str) {
	if (str == t["SIG"]) {
		return "sig"
	}
	if (str == t["MSG"]) {
		return "msg"
	}
	if (str == t["ECU"]) {
		return "ecu"
	}
	if (str == t["ENV"]) {
		return "env"
	}
	if (str == "") {
		return "db"
	}
	error(2, "unknown context " str " encountered")
}

##
# Generates a unique name for a value table entry.
#
# Updates:
# - obj_enum_count[enum, name] = (int)
#
# Sets the following fields in the given array:
# - name: A unique identifier
# - desc: The description
# - invalid: No valid identifier was in the description (bool)
# - duplicate: The identifier was already in use (bool)
#
# @param ret
#	An array to return the data set in
# @param enum
#	The identifier of the value table
# @param val
#	The value
# @param desc
#	The description string to fetch a name from
#
function getUniqueEnum(ret, enum, val, desc,
	name) {
	name = desc
	ret["invalid"] = 0
	ret["duplicate"] = 0
	sub(/[ \t\r\n].*/, "", name)
	if (name !~ /^[a-zA-Z0-9_]*$/) {
		warn("Invalid identifier '" name "' for value " sprintf("%#x", val) " in table " enum)
		gsub(/[^a-zA-Z0-9_]/, "_", name)
		warn("Replaced with '" name "'")
		ret["invalid"] = 1
	} else {
		# Name is valid, remove it from the description
		sub(/^[^ \t\r\n]+[ \t\r\n]*/, "", desc)
	}
	while (obj_enum_count[enum, name]++) {
		warn("Identifier '" name "' for value " sprintf("%#x", val) " in table " enum " already in use")
		name = name "_" sprintf("%X", val)
		warn("Replaced with '" name "'")
		ret["duplicate"] = 1
	}
	ret["name"] = name
	ret["desc"] = desc
}

##
# Discards buffered symbols until an empty line is encountered.
#
# This is used to skip the list of supported symbols at the beginning of a
# dbc file.
#
function fsm_discard() {
	fetch(":")
	fetch(rLF)
	while(fetch(rSYM "|" rLF) !~ whole(rLF)) {
		fetch(rLF)
	}
}

##
# Parse an ECU definition.
#
# Token: BU_
#
# Creates:
# - 1 obj_ecu[ecu]
# - 1 obj_ecu_db[ecu] = FILENAME
# - 1 obj_db_ecu[FILENAME, p] = ecu
#
function fsm_ecu(dummy,
	ecu, p) {
	fetch(":")
	ecu = fetch(rSYM "|" rLF)
	while (ecu !~ whole(rLF)) {
		obj_ecu[ecu]
		obj_ecu_db[ecu] = FILENAME
		p = 0
		while (obj_db_ecu[FILENAME, p++]);
		obj_db_ecu[FILENAME, --p] = ecu
		ecu = fetch(rSYM "|" rLF)
	}
}

##
# Parse a value table.
#
# Token: VAL_TABLE_
#
# Creates:
# - 1 obj_enum[enum]
# - 1 obj_enum_db[enum] = FILENAME
# - * obj_enum_val[enum, i] = val
# - * obj_enum_name[enum, i] = name
# - * obj_enum_desc[enum, i] = desc
# - * obj_enum_invalid[enum, i] = (bool)
# - * obj_enum_duplicate[enum, i] = (bool)
#
function fsm_enum(dummy,
	enum,
	val, a,
	i) {
	enum = fetch(rSYM)
	obj_enum[enum]
	obj_enum_db[enum] = FILENAME
	val = fetch(rINT "|;")
	i = 0
	while (val != ";") {
		obj_enum_val[enum, i] = val
		delete a
		getUniqueEnum(a, enum, val, fetchStr())
		obj_enum_name[enum, i] = a["name"]
		obj_enum_desc[enum, i] = a["desc"]
		obj_enum_invalid[enum, i] = a["invalid"]
		obj_enum_duplicate[enum, i] = a["duplicate"]
		++i
		val = fetch(rINT "|;")
	}
	fetch(rLF)
}

##
# Parse a value table bound to a signal.
#
# Token: VAL_
#
# Creates:
# - 1 obj_sig_enum[msgid, sig]
# - * obj_sig_enum_val[msgid, sig, i] = val
# - * obj_sig_enum_name[msgid, sig, i] = name
# - * obj_sig_enum_desc[enum, i] = desc
# - * obj_sig_enum_invalid[enum, i] = (bool)
# - * obj_sig_enum_duplicate[enum, i] = (bool)
#
function fsm_sig_enum(dummy,
	msgid, sig,
	val, a,
	i) {
	msgid = fetch(rID)
	sig = fetch(rSYM)
	obj_sig_enum[msgid, sig]
	val = fetch(rINT "|;")
	i = 0
	while (val != ";") {
		obj_sig_enum_val[msgid, sig, i] = val
		delete a
		getUniqueEnum(a, msgid SUBSEP sig, val, fetchStr())
		obj_sig_enum_name[msgid, sig, i] = a["name"]
		obj_sig_enum_desc[msgid, sig, i] = a["desc"]
		obj_sig_enum_invalid[msgid, sig, i] = a["invalid"]
		obj_sig_enum_duplicate[msgid, sig, i] = a["duplicate"]
		++i
		val = fetch(rINT "|;")
	}
	fetch(rLF)
}

##
# Parse an environment variable.
#
# Token: EV_
#
# Creates:
# - 1 obj_env[name] = val
# - 1 obj_env_type[name] = ("INT"|"FLOAT"|"DATA")
# - 1 obj_env_min[name] = (float)
# - 1 obj_env_max[name] = (float)
# - 1 obj_env_unit[name] = (string)
#
function fsm_env(dummy,
	name, a) {
	name = fetch(rSYM)
	debug("obj_env[" name "]")
	fetch(":")
	obj_env_type[name] = eTYPE[fetch(rID)]
	split(fetch(rBND), a, /[][|]/)
	obj_env_min[name] = a[2]
	obj_env_max[name] = a[3]
	obj_env_unit[name] = fetchStr()
	if (obj_env_type[name] == etINT) {
		obj_env[name] = int(fetch(rFLOAT))
	} else if (obj_env_type[name] == etFLOAT) {
		obj_env[name] = fetch(rFLOAT)
	} else {
		error(3, "environment variable type '" obj_env_type[name] "' not implemented!")
	}

	fetch(rID)  # Just a counter of environment variables
	fetch(rSYM) # DUMMY_NODE_VECTOR0
	fetch(rSYM) # Vector__XXX
	fetch(";")
}

##
# Parse the data length count of DATA type environment variables.
#
# Token: ENVVAR_DATA_
#
# Creates:
# - 1 obj_env_dlc[name] = (int)
#
function fsm_env_data(dummy,
name) {
	name = fetch(rSYM)
	debug("obj_env_dlc[" name "]")
	fetch(":")
	obj_env_dlc[name] = int(fetch(rDLC))
	fetch(";")
}

##
# Parse a message definition.
#
# Token: BO_
#
# Creates:
# - 1 obj_msg[id]
# - 1 obj_msg_name[id] = name
# - 1 obj_msg_dlc[id] = dlc
# - 1 obj_msg_tx[id] = ecu
# - 1 obj_ecu_tx[ecu, i] = id
#
function fsm_msg(dummy,
	id,
	name,
	dlc,
	ecu,
	i) {
	id = int(fetch(rID))
	debug("obj_msg[" id "]")
	name = fetch(rSYM)
	fetch(":")
	dlc = int(fetch(rDLC))
	ecu = fetch(rSYM)
	fetch(rLF)

	obj_msg[id]
	obj_msg_name[id] = name
	obj_msg_dlc[id] = dlc

	if (ecu in obj_ecu) {
		obj_msg_tx[id] = ecu
		while (obj_ecu_tx[ecu, i++]);
		obj_ecu_tx[ecu, --i] = id
	}

	while (fetch(rSYM) == t["SIG"]) {
		fsm_sig(id)
	}
}

##
# Parse a signal definition.
#
# Token: SG_
#
# Creates:
# - 1 obj_sig[msgid, name]
# - 1 obj_sig_name[msgid, name] = name
# - 1 obj_sig_msgid[msgid, name] = msgid
# - 1 obj_sig_multiplexor[msgid, name] = (bool)
# - 1 obj_sig_multiplexed[msgid, name] = (int)
# - 1 obj_sig_sbit[msgid, name] = (uint)
# - 1 obj_sig_len[msgid, name] = (uint)
# - 1 obj_sig_intel[msgid, name] = (bool)
# - 1 obj_sig_signed[msgid, name] = (bool)
# - 1 obj_sig_fac[msgid, name] = (float)
# - 1 obj_sig_off[msgid, name] = (float)
# - 1 obj_sig_min[msgid, name] = (float)
# - 1 obj_sig_max[msgid, name] = (float)
# - 1 obj_sig_unit[msgid, name] = (string)
# - * obj_sig_rx[msgid, name, i] = ecu
# - * obj_ecu_rx[ecu, p] = msgid, name
# - * obj_msg_sig[msgid, p] = msgid, name
#
function fsm_sig(msgid,
	name,
	multiplexing,
	a,
	ecu, i, p) {
	name = fetch(rSYM)
	debug("obj_sig[" msgid ", " name "]")
	obj_sig[msgid, name]
	obj_sig_name[msgid, name] = name
	obj_sig_msgid[msgid, name] = msgid
	while (obj_msg_sig[msgid, p++]);
	obj_msg_sig[msgid, --p] = msgid SUBSEP name
	multiplexing = fetch("m[0-9]+|M")
	obj_sig_multiplexor[msgid, name] = (multiplexing == "M")
	gsub(/[mM]/, "", multiplexing)
	obj_sig_multiplexed[msgid, name] = multiplexing
	fetch(":")
	split(fetch(rSIG), a, /[|@]/)
	obj_sig_sbit[msgid, name] = a[1]
	obj_sig_len[msgid, name] = a[2]
	obj_sig_intel[msgid, name] = (a[3] ~ /^1/)
	obj_sig_signed[msgid, name] = (a[3] ~ /-$/)
	split(fetch(rVEC), a, /[(),]/)
	obj_sig_fac[msgid, name] = a[2]
	obj_sig_off[msgid, name] = a[3]
	split(fetch(rBND), a, /[][|]/)
	obj_sig_min[msgid, name] = a[2]
	obj_sig_max[msgid, name] = a[3]
	obj_sig_unit[msgid, name] = fetchStr()
	split(fetch(rSYMS), a, /,/)
	for (ecu in a) {
		if (a[ecu] in obj_ecu) {
			obj_sig_rx[msgid, name, i++] = a[ecu]
			p = 0
			while (obj_ecu_rx[a[ecu], p++]);
			obj_ecu_rx[a[ecu], --p] = msgid SUBSEP name
		}
	}
	fetch(rLF)
}

##
# Parse comments.
#
# Token: CM_
#
# Creates one of:
# - 1 obj_db_comment[FILENAME]
# - 1 obj_ecu_comment[name]
# - 1 obj_env_comment[name]
# - 1 obj_msg_comment[msgid]
# - 1 obj_sig_comment[msgid, name]
#
function fsm_comment(dummy,
	context, name, msgid, str) {
	context = getContext(fetch(rSYM))
	if (context == "db") {
		obj_db_comment[FILENAME] = fetchStr()
	} else if (context == "env") {
		name = fetch(rSYM)
		obj_env_comment[name] = fetchStr()
	} else if (context == "ecu") {
		name = fetch(rSYM)
		obj_ecu_comment[name] = fetchStr()
	} else if (context == "msg") {
		msgid = fetch(rID)
		obj_msg_comment[msgid] = fetchStr()
	} else if (context == "sig") {
		msgid = fetch(rID)
		name = fetch(rSYM)
		obj_sig_comment[msgid, name] = fetchStr()
	} else {
		error(10, "comment for '" context "' not implemented!")
	}
	fetch(";")
	fetch(rLF)
}

##
# Parse a custom attribute definition.
#
# Token: BA_DEF_
#
# Creates:
# - 1 obj_attr[name]
# - 1 obj_attr_context[name] = ("sig"|"msg"|"ecu"|"env"|"db")
# - 1 obj_attr_type[name] = ("INT"|"ENUM"|"STRING")
# - ? obj_attr_min[name] = (float)
# - ? obj_attr_max[name] = (float)
# - * obj_attr_enum[name, i] = (string)
# - ? obj_attr_str[name] = (string)
#
function fsm_attrrange(dummy,
	context,
	name,
	type,
	val,
	i) {
	context = getContext(fetch(rSYM))
	name = fetchStr()
	obj_attr_context[name] = context
	obj_attr[name]
	type = fetch(rSYM)
	obj_attr_type[name] = type
	if (type in atNUM) {
		obj_attr_min[name] = fetch(rFLOAT)
		obj_attr_max[name] = fetch(rFLOAT)
	} else if (type == atENUM) {
		val = fetchStr()
		while (val != "") {
			obj_attr_enum[name, ++i] = val
			fetch(",")
			val = fetchStr()
		}
	} else if (type == atSTR) {
		obj_attr_str[name] = fetchStr()
	}
	fetch(";")
}

##
# Parse a custom relation attribute definition.
#
# Token: BA_DEF_REL_
#
# Creates:
# - 1 obj_attr[name]
# - 1 obj_attr_context[name] = "rel"
# - 1 obj_attr_from[name] = ("sig"|"msg"|"ecu"|"env"|"db")
# - 1 obj_attr_to[name] = ("sig"|"msg"|"ecu"|"env"|"db")
# - 1 obj_attr_type[name] = ("INT"|"ENUM"|"STRING")
# - ? obj_attr_min[name] = (float)
# - ? obj_attr_max[name] = (float)
# - * obj_attr_enum[name, i] = (string)
# - ? obj_attr_str[name] = (string)
#
function fsm_relattrrange(dummy,
	context, relation,
	name,
	type,
	val,
	i) {
	relation = fetch(rSYM)
	name = fetchStr()
	obj_attr_context[name] = "rel"
	obj_attr[name]
	sub(/REL_$/, "", relation)
	context = relation
	sub(/[A-Z]+_$/, "", context)
	obj_attr_from[name] = getContext(context)
	context = relation
	sub(/^[A-Z]+_/, "", context)
	obj_attr_to[name] = getContext(context)
	type = fetch(rSYM)
	obj_attr_type[name] = type
	if (type in atNUM) {
		obj_attr_min[name] = fetch(rFLOAT)
		obj_attr_max[name] = fetch(rFLOAT)
	} else if (type == atENUM) {
		val = fetchStr()
		while (val != "") {
			obj_attr_enum[name, ++i] = val
			fetch(",")
			val = fetchStr()
		}
	} else if (type == atSTR) {
		obj_attr_str[name] = fetchStr()
	}
	fetch(";")
}

##
# Parse attribute default value.
#
# Token: BA_DEF_DEF_
#
# Creates:
# - 1 obj_attr_default[name] = value
# - * obj_msg_attr[msgid, name]
# - * obj_sig_attr[msgid, signame, name]
# - * obj_db_attr[FILENAME, name]
#
function fsm_attrdefault(dummy,
	name,
	value,
	i) {
	name = fetchStr()
	if (obj_attr_type[name] == atSTR) {
		obj_attr_default[name] = fetchStr()
	} else if (obj_attr_type[name] == atENUM) {
		value = fetchStr()
		while (obj_attr_enum[name, ++i] != value);
		obj_attr_default[name] = i
	} else if (obj_attr_type[name] in atNUM) {
		obj_attr_default[name] = fetch(rFLOAT)
	} else {
		error(4, "attribute type '" obj_attr_type[name] "' not implemented!")
	}
	debug("obj_attr_default[" name "] = " obj_attr_default[name])
	fetch(";")
	# Poplulate objects with defaults
	if (obj_attr_context[name] == "msg") {
		for (i in obj_msg) {
			obj_msg_attr[i, name] = obj_attr_default[name]
		}
	} else if (obj_attr_context[name] == "sig") {
		for (i in obj_sig) {
			obj_sig_attr[i, name] = obj_attr_default[name]
		}
	} else if (obj_attr_context[name] == "db") {
		for (i in obj_db) {
			obj_db_attr[i, name] = obj_attr_default[name]
		}
	}
}

##
# Fetches an attribute value of a given type from the read buffer.
#
# @param attribute
#	The attribute type identifier
# @return
#	The value of the chosen type
#
function fetch_attrval(attribute) {
	if (obj_attr_type[attribute] == atSTR) {
		return fetchStr()
	} else if (obj_attr_type[attribute] == atENUM) {
		return int(fetch(rINT))
	} else if (obj_attr_type[attribute] == atINT) {
		return int(fetch(rFLOAT))
	}
	return fetch(rFLOAT)
}

##
# Parse an attribute value.
#
# Token: BA_
#
# Creates one of:
# - 1 obj_sig_attr[msgid, signame, name] = value
# - 1 obj_msg_attr[msgid, name] = value
# - 1 obj_ecu_attr[ecu, name] = value
# - 1 obj_db_attr[FILENAME, name] = value
#
function fsm_attr(dummy,
	name,
	id, sig) {
	name = fetchStr()
	if (obj_attr_context[name] == "sig") {
		fetch(t["SIG"])
		id = fetch(rID) # Fetch message ID
		sig = fetch(rSYM)
		obj_sig_attr[id, sig, name] = fetch_attrval(name)
		debug("obj_sig_attr[" id ", " sig ", " name "] = " obj_sig_attr[id, sig, name])
	}
	else if (obj_attr_context[name] == "msg") {
		fetch(t["MSG"])
		id = fetch(rID)
		obj_msg_attr[id, name] = fetch_attrval(name)
		debug("obj_msg_attr[" id "," name "] = " obj_msg_attr[id, name])
	}
	else if (obj_attr_context[name] == "ecu") {
		fetch(t["ECU"])
		id = fetch(rSYM)
		obj_ecu_attr[id, name] = fetch_attrval(name)
	}
	else if (obj_attr_context[name] == "db") {
		obj_db_attr[FILENAME, name] = fetch_attrval(name)
		debug("obj_db_attr[" FILENAME "," name "] = " obj_db_attr[FILENAME, name])
	}
	else {
		error(6, "attributes for " obj_attr_context[name] " not implemented!")
	}
	fetch(";")
}

##
# Parse a relation attribute value.
#
# Token: BA_REL_
#
# Creates:
# - 1 obj_rel_attr[name, from, to] = value
# - 1 obj_rel_attr_name[name, from, to] = name
# - 1 obj_rel_attr_from[name, from, to] = from
# - 1 obj_rel_attr_to[name, from, to] = to
#
# The types of to and from are recorded in:
# - obj_attr_from[name]
# - obj_attr_to[name]
#
function fsm_relattr(dummy,
	name,
	from, to) {
	name = fetchStr()
	fetch(rSYM) # Fetch the type of the relation, this is already known
	if (obj_attr_from[name] == "sig") {
		from = fetch(rID) # Fetch message ID
		from = from SUBSEP fetch(rSYM)
	}
	else if (obj_attr_from[name] == "msg") {
		from = fetch(rID)
	}
	else if (obj_attr_from[name] == "ecu") {
		from = fetch(rSYM)
	}
	else if (obj_attr_from[name] == "db") {
		from = FILENAME
	}
	else {
		error(7, "relation attributes for " obj_attr_from[name] " not implemented!")
	}
	fetch(rSYM) # Fetch the type of the related object
	if (obj_attr_to[name] == "sig") {
		to = fetch(rID) # Fetch message ID
		to = to SUBSEP fetch(rSYM)
	}
	else if (obj_attr_to[name] == "msg") {
		to = fetch(rID)
	}
	else if (obj_attr_to[name] == "ecu") {
		to = fetch(rSYM)
	}
	else if (obj_attr_to[name] == "db") {
		to = FILENAME
	}
	else {
		error(8, "relation attributes for " obj_attr_to[name] " not implemented!")
	}
	obj_rel_attr[name, from, to] = fetch_attrval(name)
	debug("obj_rel_attr[" name ", " from ", " to "] = " obj_rel_attr[name, from, to])
	obj_rel_attr_name[name, from, to] = name
	obj_rel_attr_from[name, from, to] = from
	obj_rel_attr_to[name, from, to] = to
	fetch(";")
}

##
# Parse the symbol table at the beginning of a .dbc file, bail if
# unsupported symbols are encountered.
#
# Token: NS_
#
function fsm_symbols(dummy,
	sym, i) {
	fetch(":")
	fetch(rLF)
	while (sym = fetch(rSYM)) {
		fetch(rLF)
		for (i in t) {
			if (t[i] == sym) {
				break
			}
		}
		if (t[i] != sym) {
			error(5, "unknown symbol " sym " encountered")
		}
	}
}

##
# Gets a list of ECUs that transmit a certain message.
#
# This may appear when several device options are available.
#
# Token: BO_TX_BU_
#
# Creates:
# - 1 obj_ecu_tx[ecu, i] = msgid
#
function fsm_tx(dummy,
	msgid, ecus, i, p, present) {
	msgid = fetch(rID)
	fetch(":")
	split(",", names, fetch(rSYMS))
	for (i in ecus) {
		p = 0
		present = 0
		while (obj_ecu_tx[ecus[i], p]) {
			if (obj_ecu_tx[ecus[i], p++] == msgid) {
				present = 1
			}
		}
		if (!present) {
			obj_ecu_tx[ecus[i], p++] = msgid
		}
	}
	fetch(";")
}

##
# Gets a signal group.
#
# Token: SIG_GROUP_
#
# Creates:
# - 1 obj_siggrp[msgid, name] = name
# - 1 obj_siggrp_msg[msgid, name] = msgid
# - * obj_siggrp_sig[msgid, name, i] = sig
# - * obj_sig_grp[msgid, sig, p] = msgid, name
# - * obj_msg_grp[msgid, p] = msgid, name
#
function fsm_siggrp(dummy,
	i, msgid, name, sig, p) {
	msgid = fetch(rID)
	name = fetch(rSYM)
	obj_siggrp[msgid, name] = name
	debug("obj_siggrp[" msgid ", " name "] = " obj_siggrp[msgid, name])
	i = fetch(rID) # Seems to always be 1
	if (i != 1) {
		warn("Signal group " msgid ", " name " has unknown properties")
	}
	obj_siggrp_msg[msgid, name] = msgid
	fetch(":")
	i = 0
	while (!fetch(";")) {
		sig = fetch(rSYM)
		obj_siggrp_sig[msgid, name, i++] = msgid SUBSEP sig
		p = 0
		while (obj_sig_grp[msgid, sig, p++]);
		obj_sig_grp[msgid, sig, --p] = msgid SUBSEP name
	}
	p = 0
	while (obj_msg_grp[msgid, p++]);
	obj_msg_grp[msgid, --p] = msgid SUBSEP name
}

##
# Pick tokens from the input buffer and call the respective parsing
# functions.
#
# Creates:
# - 1 obj_db[FILENAME]
#
function fsm_start(dummy,
	sym) {
	sym = fetch(rSYM "|" rLF)

	obj_db[FILENAME]

	# Check whether symbol is known but not supported
	if (t[sym]) {
		error(9, "unsupported symbol " sym " encountered")
	}
	# Check known symbols
	if (sym == t["SYMBOLS"]) {
		fsm_symbols()
	}
	# Discard version
	else if (sym == t["VER"]) {
		fetchStr()
	}
	# Discard not required symbols
	else if (sym in tDISCARD) {
		fsm_discard()
	}
	# Environment variables
	else if (sym == t["ENV"]) {
		fsm_env()
	}
	# Get ECUs
	else if (sym == t["ECU"]) {
		fsm_ecu()
	}
	# Get signal enums
	else if (sym == t["SIG_ENUM"]) {
		fsm_sig_enum()
	}
	# Get enums
	else if (sym == t["ENUM"]) {
		fsm_enum()
	}
	# Get message objects
	else if (sym == t["MSG"]) {
		fsm_msg()
	}
	# Get comments
	else if (sym == t["COM"]) {
		fsm_comment()
	}
	# Get attribute ranges
	else if (sym == t["ATTRRANGE"]) {
		fsm_attrrange()
	}
	# Get relation attribute ranges
	else if (sym == t["RELATTRRANGE"]) {
		fsm_relattrrange()
	}
	# Get attribute defaults
	else if (sym == t["ATTRDEFAULT"]) {
		fsm_attrdefault()
	}
	# Get relation attribute defaults
	else if (sym == t["RELATTRDEFAULT"]) {
		fsm_attrdefault()
	}
	# Get attributes
	else if (sym == t["ATTR"]) {
		fsm_attr()
	}
	# Get relational attributes
	else if (sym == t["RELATTR"]) {
		fsm_relattr()
	}
	# Get the DLC of environment variables of type DATA
	else if (sym == t["EDLC"]) {
		fsm_env_data()
	}
	# Get a list of TX ECUs for a message
	else if (sym == t["TX"]) {
		fsm_tx()
	}
	# Get a signal group
	else if (sym == t["SIG_GRP"]) {
		fsm_siggrp()
	}
}

##
# This starts the line wise parsing of the DBC file.
#
{
	while ($0) {
		fsm_start()
	}
}

#
# End of the parser section, the following code deals with output.
#

##
# Returns a string contining all elements of an array connected by a
# seperator.
#
# @param sep
#	The seperator to use between concatenated strings
# @param array
#	The array to concatenate
# @return
#	A string containing all array elements
#
function join(sep, array,
	i, result, a) {
	for (i in array) {
		result = result (a++ ? sep : "") array[i]
	}
	return result
}

##
# Returns a string contining all indexes of an array connected by a
# seperator.
#
# @param sep
#	The seperator to use between concatenated strings
# @param array
#	The array to concatenate
# @return
#	A string contining the indexes of all array elements
#
function joinIndex(sep, array,
	i, result, a) {
	for (i in array) {
		result = result (a++ ? sep : "") i
	}
	return result
}

##
# Returns the greatest common divider (GCD).
#
# @param a
#	An integer
# @param b
#	An integer
# @return
#	The greatest common divider of a and b
#
function euclid(a, b,
	tmp) {
	while (b != 0) {
		tmp = a
		a = b
		b = tmp % b
	}
	return a
}

##
# Returns a compact string representation of a rational number.
#
# @param n
#	The numerator
# @param d
#	The denominator
# @return
#	The given rational number as a string
#
function rationalFmt(n, d) {
	# Zero always wins
	if (n == 0) {
		return "* 0"
	}
	# The given rational is an integer
	if (d == 1) {
		if (n > 0) {
			return sprintf("* %d", n)
		}
		return sprintf("* (%d)", n)
	}
	# There only is a division factor
	if (n == 1) {
		if (d > 0) {
			return sprintf("/ %d", d)
		}
		return sprintf("/ (%d)", d)
	}
	# Both values are positive
	if (n > 0 && d > 0) {
		return sprintf("* %d / %d", n, d)
	}
	# One is always supposed to be positive
	if (n > 0) {
		return sprintf("* %d / (%d)", n, d)
	}
	if (d > 0) {
		return sprintf("* (%d) / %d", n, d)
	}
	return sprintf("* (%d) / (%d)", n, d)
}

##
# Returns a rational string representation of a real value.
#
# This function builds the value around the numerator.
#
# @param val
#	The real value to return as a rational
# @param base
#	The logical number base to generate the rational from
# @param precision
#	The maximum number of bits for either rational component
# @return
#	A rational string representation of the given value
#
function rationalN(val, base, precision,
	div, gcd) {
	div = 1
	while((val % 1) != 0 && div < 2^precision) {
		div *= base
		val *= base
	}
	gcd = euclid(int(val), int(div))
	val /= gcd
	div /= gcd
	while ((val > 0 ? val : -val) >= 2^precision || (div > 0 ? div : -div) >= 2^precision) {
		val /= base
		div /= base
	}
	return rationalFmt(int(val), int(div))
}

##
# Returns a rational string representation of a real value.
#
# This function builds the value around the denominator.
#
# @param val
#	The real value to return as a rational
# @param base
#	The logical number base to generate the rational from
# @param precision
#	The maximum number of bits for either rational component
# @return
#	A rational string representation of the given value
#
function rationalD(val, base, precision,
	div, gcd) {
	if (val == 0) {
		return rationalFmt(0)
	}
	div = 1 / val
	val = 1
	while((div % 1) != 0 && val < 2^precision) {
		div *= base
		val *= base
	}
	gcd = euclid(int(val), int(div))
	val /= gcd
	div /= gcd
	while ((val > 0 ? val : -val) >= 2^precision || (div > 0 ? div : -div) >= 2^precision) {
		val /= base
		div /= base
	}
	return rationalFmt(int(val), int(div))
}

##
# Returns a rational string representation of a real value.
#
# This uses the different rational*() functions to find a minimal
# representation of the value.
#
# @param val
#	The real value to return as a rational
# @param precision
#	The maximum number of bits for either rational component
# @return
#	A rational string representation of the given value
#
function rational(val, precision,
	a, b) {
	a = rationalN(val, 2, precision)
	b = rationalD(val, 2, precision)
	a = (length(b) < length(a) ? b : a)
	b = rationalN(val, 10, precision)
	a = (length(b) < length(a) ? b : a)
	b = rationalD(val, 10, precision)
	a = (length(b) < length(a) ? b : a)
	return a
}

##
# Applies filter chains to a given string.
#
# Filters are a colon separated lists of the following filter commands:
# | Command | Effect
# |---------|----------------------------------------
# | low     | Convert to lower case
# | up      | Convert to upper case
# | camel   | Convert to camel case
# | uncamel | Convert camel case to _ separated
# | %...    | A printf(3) style format specification
#
# @param str
#	The string to apply the filters to
# @param filters
#	The list of filters
# @param template
#	The name of the current template
# @return
#	The converted string
#
function filter(str, filters, template,
	cmds, count, i) {
	count = split(filters, cmds, ":")
	for (i = 1; i <= count; ++i) {
		if (cmds[i] == "low") {
			str = tolower(str)
			continue
		}
		if (cmds[i] == "up") {
			str = toupper(str)
			continue
		}
		if (cmds[i] == "camel") {
			str = tolower(str)
			while (match(str, /_./)) {
				str = substr(str, 1, RSTART - 1) \
				      toupper(substr(str, RSTART + 1, 1)) \
				      substr(str, RSTART + 2)
			}
			continue
		}
		if (cmds[i] == "uncamel") {
			while (match(str, /[A-Z]+/)) {
				str = substr(str, 1, RSTART - 1) "_" \
				      tolower(substr(str, RSTART, RLENGTH)) \
				      substr(str, RSTART + RLENGTH)
			}
			continue
		}
		if (cmds[i] ~ /^%[-#+ 0]*[0-9]*\.?[0-9]*[diouxXfFeEgGaAcsb]$/) {
			str = sprintf(cmds[i], str)
			continue
		}

		if (!UNKNOWN[template, cmds[i]]++) {
			warn(template ": Unknown filter '" cmds[i] "' ignored")
		}
	}
	return str
}

##
# Populates a template line with data.
#
# Multiline data in a template needs to be in its own line.
#
# Lines with empty data fields are removed.
#
# Identifiers in templates have the following shape:
#	"<:" name ":>"
#
# Additionally boolean filters can be installed:
#	"<?" name "?>"
#
# If the variable addressed in the filter evaluetes to true, the filter
# is removed, otherwise the entire line is removed.
#
# @param data
#	The array containing field data
# @param line
#	The line to perform substitutions in
# @param template
#	The name of the template this line comes from, this is used to warn
#	about deprecated arguments
# @return
#	The line(s) with performed substitutions
#
function tpl_line(data, line, template,
	name, filters, pre, post, count, array, i) {
	# Filter lines with boolean checks
	while(match(line, /<\?[a-zA-Z0-9]+\?>/)) {
		name = substr(line, RSTART + 2, RLENGTH - 4)
		if (!data[name]) {
			return ""
		}
		line = substr(line, 1, RSTART - 1) substr(line, RSTART + RLENGTH)
	}
	# Insert data
	while(match(line, /<:[a-zA-Z0-9]+(:[^>:][^:]*)*:>/)) {
		filters = name = substr(line, RSTART + 2, RLENGTH - 4)
		sub(/:.*/, "", name)
		sub(/^[^:]*:?/, "", filters)
		pre = substr(line, 1, RSTART - 1)
		post = substr(line, RSTART + RLENGTH)
 		# Warn when using deprecated symbols
		if (data["#" name] && !DEPRECATED[template, name]++) {
			warn(template ": <:" name ":> is deprecated in favour of <:" data["#" name] ":>")
		}
		# Output the template line once for each data value
		count = split(data[name], array, RS)
		while (count && array[count] ~ /^[ \t\r\n]*$/ && --count);
		if (!count) {
			return ""
		}
		line = pre filter(array[1], filters, template) post
		for (i = 2; i <= count; i++) {
			line = line ORS pre filter(array[i], filters, template) post
		}
	}
	return line ORS
}

##
# Reads a template, substitutes place holders with data from a given
# array and returns it.
#
# @param data
#	The array to take data from
# @param name
#	The name of the template file
# @return
#	The filled up template
#
function template(data, name,
	buf, line) {
	# Read template
	name = TEMPLATES name
	# Warn about missing templates, but do not terminate
	if ((getline line < name) < 0) {
		if (!MISSING[name]++) {
			warn("Template " name " is missing")
		}
		return
	}
	buf = tpl_line(data, line, name)
	while (getline line < name) {
		buf = buf tpl_line(data, line, name)
	}
	close(name)
	return buf
}

##
# Set the necessary type to be able to shift something to the given bit.
#
# Creates the entries int8, int16 and int32 in the given arrays, with the
# fitting type set to the value 1 and the others to 0.
#
# @param array
#	The array put the entries into
# @param bitpos
#	The bit that needs to be addressable
#
function setTypes(array, bitpos) {
	array["int32"] = (bitpos >= 16 ? 1 : 0)
	array["int16"] = (!array["int32"] && bitpos >= 8 ? 1 : 0)
	array["int8"] = (bitpos < 8 ? 1 : 0)
}

##
# Returns a unique signal identifier using the sigident.tpl file.
#
# Returns an empty string if the template is missing.
#
# @param sig
#	The signal reference consisting of message and signal name
# @return
#	A unique signal identifier
#
function sigident(sig,
	tpl) {
	tpl["sig"] = obj_sig_name[sig]
	tpl["msgid"] = sprintf("%x", obj_sig_msgid[sig])
	tpl["#msgid"] = "msg"
	tpl["msg"] = obj_sig_msgid[sig]
	tpl["msgname"] = obj_msg_name[obj_sig_msgid[sig]]
	sig = template(tpl, "sigid.tpl")
	sub(ORS "$", "", sig)
	return sig
}

##
# Returns a unique signal group identifier using the sigident.tpl file.
#
# Returns an empty string if the template is missing.
#
# @param sg
#	The signal group reference consisting of message and signal name
# @return
#	A unique signal identifier
#
function siggrpident(sg,
	tpl) {
	tpl["sig"] = obj_siggrp[sg]
	tpl["msgid"] = sprintf("%x", obj_siggrp_msg[sg])
	tpl["#msgid"] = "msg"
	tpl["msg"] = obj_siggrp_msg[sg]
	tpl["msgname"] = obj_msg_name[obj_siggrp_msg[sg]]
	sg = template(tpl, "sigid.tpl")
	sub(ORS "$", "", sg)
	return sg
}

##
# Generates a printable message id be removing the extended bit.
#
# @param id
#	The message id to return
# @return
#	The message id without the extended bit
#
function msgid(id) {
	return int(msgidext(id) ? id - 2^31 : id)
}

##
# Tests a message id for the extended bit.
#
# @param id
#	The message id to check
# @retval 1
#	The message is extended
# @retval 0
#	The message is not extended
#
function msgidext(id) {
	return int(id) >= 2^31
}

##
# Print the DBC files to stdout.
#
END {
	# Do not produce output on error
	if (errno) {
		exit errno
	}

	# Headers
	tpl["date"] = DATE
	for (db in obj_db) {
		sub(/.*\//, "", db)
		sub(/\.[^\.]*$/, "", db)
		dbs[db]
	}
	tpl["db"] = joinIndex(RS, dbs)
	printf("%s", template(tpl, "header.tpl"))

	# Introduce the DB files
	for (file in obj_db) {
		delete tpl
		tpl["db"] = file
		sub(/.*\//, "", tpl["db"])
		sub(/\.[^\.]*$/, "", tpl["db"])
		tpl["file"] = file
		tpl["comment"] = obj_db_comment[file]
		delete ecus
		p = 0
		while (obj_db_ecu[file, p]) {
			ecus[obj_db_ecu[file, p++]]
		}
		tpl["ecu"] = joinIndex(RS, ecus)
		printf("%s", template(tpl, "file.tpl"))
	}

	# Introduce the ECUs
	for (ecu in obj_ecu) {
		delete tpl
		tpl["ecu"] = ecu
		tpl["comment"] = obj_ecu_comment[ecu]
		# DB
		tpl["db"] = obj_ecu_db[ecu]
		sub(/.*\//, "", tpl["db"])
		sub(/\.[^\.]*$/, "", tpl["db"])
		# TX messages
		p = 0
		delete tx
		delete txid
		delete txname
		while (obj_ecu_tx[ecu, p]) {
			txname[obj_msg_name[obj_ecu_tx[ecu, p]]]
			txid[msgid(obj_ecu_tx[ecu, p])]
			tx[sprintf("%x", msgid(obj_ecu_tx[ecu, p++]))]
		}
		tpl["txname"] = joinIndex(RS, txname)
		tpl["txid"] = joinIndex(RS, txid)
		tpl["tx"] = joinIndex(RS, tx)
		tpl["#tx"] = "txid"
		# RX signals
		p = 0
		delete rxid
		delete rx
		while (obj_ecu_rx[ecu, p]) {
			rxid[sigident(obj_ecu_rx[ecu, p])]
			rx[obj_sig_name[obj_ecu_rx[ecu, p++]]]
		}
		tpl["rxid"] = joinIndex(RS, rxid)
		tpl["rx"] = joinIndex(RS, rx)
		# Load template
		printf("%s", template(tpl, "ecu.tpl"))
	}

	# Introduce the Messages
	for (msg in obj_msg) {
		delete tpl
		tpl["id"] = sprintf("%x", msgid(msg))
		tpl["msg"] = msgid(msg)
		tpl["#id"] = "msg"
		tpl["name"] = obj_msg_name[msg]
		tpl["comment"] = obj_msg_comment[msg]
		tpl["ecu"] = obj_msg_tx[msg]
		tpl["ext"] = msgidext(msg)
		tpl["dlc"] = obj_msg_dlc[msg]
		tpl["cycle"] = 0 + obj_msg_attr[msg, aCYCLE]
		tpl["fast"] = 0 + obj_msg_attr[msg, aFCYCLE]
		tpl["delay"] = 0 + obj_msg_attr[msg, aDELAY]
		tpl["send"] = obj_msg_attr[msg, aSEND]
		# Get signal list
		i = 0
		delete sigids
		delete sigs
		while (obj_msg_sig[msg, i]) {
			sigids[sigident(obj_msg_sig[msg, i])]
			sigs[obj_sig_name[obj_msg_sig[msg, i++]]]
		}
		tpl["sigid"] = joinIndex(RS, sigids)
		tpl["sig"] = joinIndex(RS, sigs)
		# Get signal group list
		i = 0
		delete sgids
		delete sgnames
		while (obj_msg_grp[msg, i]) {
			sgids[siggrpident(obj_msg_grp[msg, i])]
			sgnames[obj_siggrp[obj_msg_grp[msg, i++]]]
		}
		tpl["sgid"] = joinIndex(RS, sgids)
		tpl["sgname"] = joinIndex(RS, sgnames)
		# Load template
		printf("%s", template(tpl, "msg.tpl"))
	}

	# Introduce signal groups
	for (id in obj_siggrp) {
		delete tpl
		tpl["id"] = siggrpident(id)
		tpl["name"] = obj_siggrp[id]
		tpl["msgid"] = sprintf("%x", msgid(obj_siggrp_msg[id]))
		tpl["msg"] = msgid(obj_siggrp_msg[id])
		tpl["#msgid"] = "msg"
		tpl["msgname"] = obj_msg_name[obj_siggrp_msg[id]]
		sub(/.*\//, "", tpl["db"])
		sub(/\.[^\.]*$/, "", tpl["db"])
		# Signals
		delete sigids
		delete sigs
		p = 0
		while (obj_siggrp_sig[id, p]) {
			sigids[sigident(obj_siggrp_sig[id, p])]
			sigs[obj_sig_name[obj_siggrp_sig[id, p++]]]
		}
		tpl["sigid"] = joinIndex(RS, sigids)
		tpl["sig"] = joinIndex(RS, sigs)
		# Load template
		printf("%s", template(tpl, "siggrp.tpl"))
	}

	# Types of integer bits
	delete inttypes
	inttypes[8]
	inttypes[16]
	inttypes[32]
	# Introduce the Signals
	for (sig in obj_sig) {
		delete tpl
		tpl["name"] = obj_sig_name[sig]
		tpl["id"] = sigident(sig)
		tpl["comment"] = obj_sig_comment[sig]
		# Reference the message this signal belongs to
		tpl["msgid"] = sprintf("%x", msgid(obj_sig_msgid[sig]))
		tpl["msg"] = msgid(obj_sig_msgid[sig])
		tpl["#msgid"] = "msg"
		tpl["msgname"] = obj_msg_name[obj_sig_msgid[sig]]
		# Reference the signal groups this message belongs to
		i = 0
		delete grpids
		delete grps
		while (obj_sig_grp[sig, i]) {
			grpids[siggrpident(obj_sig_grp[sig, i])]
			grps[obj_siggrp[obj_sig_grp[sig, i++]]]
		}
		tpl["sgid"] = joinIndex(RS, grpids)
		tpl["sgname"] = joinIndex(RS, grps)
		# Reference the RX ECUs
		i = 0
		delete ecus
		while (obj_sig_rx[sig, i]) {
			ecus[obj_sig_rx[sig, i++]]
		}
		tpl["ecu"] = joinIndex(RS, ecus)
		# Has value table?
		tpl["enum"] = (sig in obj_sig_enum)
		# Tuple
		tpl["intel"] = obj_sig_intel[sig]
		tpl["motorola"] = !obj_sig_intel[sig]
		tpl["signed"] = obj_sig_signed[sig]
		tpl["sbit"] = obj_sig_sbit[sig]
		tpl["len"] = obj_sig_len[sig]
		# Setup/Start
		tpl["start"] = 0 + obj_sig_attr[sig, aSTART]
		# Calc
		fac = rational(obj_sig_fac[sig], 16)
		off = rational(obj_sig_off[sig], 16)
		tpl["calc16"] = "(x) * fmt"
		if (fac != "* 1") {
			tpl["calc16"] = tpl["calc16"] " " fac
		}
		if (off != "* 0") {
			tpl["calc16"] = tpl["calc16"] " + fmt " off
		}
		# Min, max, offset
		tpl["min"] = sprintf("%.0f", (obj_sig_min[sig] - obj_sig_off[sig]) / obj_sig_fac[sig])
		tpl["max"] = sprintf("%.0f", (obj_sig_max[sig] - obj_sig_off[sig]) / obj_sig_fac[sig])
		tpl["off"] = sprintf("%.0f", obj_sig_off[sig] / obj_sig_fac[sig])
		# Getter and setter
		bits = obj_sig_len[sig]
		bpos = obj_sig_sbit[sig]
		if (obj_sig_intel[sig]) {
			# Intel style signals
			pos = 0
			tpl["getbuf"] = ""
			tpl["setbuf"] = ""
			if (obj_sig_signed[sig] && !(bits in inttypes)) {
				delete sbits
				sbits["sign"] = "-"
				sbits["byte"] = int((bpos + bits - 1) / 8)
				sbits["align"] = (bpos + bits - 1) % 8
				sbits["mask"] = "0x01"
				sbits["msk"] = 1
				sbits["#mask"] = "msk"
				sbits["pos"] = bits
				setTypes(sbits, sbits["pos"])
				tpl["getbuf"] = template(sbits, "sig_getbuf.tpl")
			}
			while (bits > 0) {
				delete sbits
				sbits["sign"] = "+"
				sbits["byte"] = int(bpos / 8)
				sbits["align"] = bpos % 8
				shift = 8 - sbits["align"]
				if (shift > bits) {
					shift = bits
				}
				sbits["mask"] = sprintf("%#04x", 2^shift - 1)
				sbits["msk"] = 2^shift - 1
				sbits["#mask"] = "msk"
				sbits["pos"] = pos
				setTypes(sbits, sbits["pos"] + shift - 1)
				tpl["getbuf"] = tpl["getbuf"] \
				                template(sbits, "sig_getbuf.tpl")
				tpl["setbuf"] = tpl["setbuf"] \
				                template(sbits, "sig_setbuf.tpl")
				pos += shift
				bpos += shift
				bits -= shift
			}
		} else {
			# Motorola signals
			tpl["getbuf"] = ""
			tpl["setbuf"] = ""
			if (obj_sig_signed[sig] && !(bits in inttypes)) {
				delete sbits
				sbits["sign"] = "-"
				sbits["byte"] = int(bpos / 8)
				sbits["align"] = bpos % 8
				sbits["mask"] = "0x01"
				sbits["msk"] = 1
				sbits["#mask"] = "msk"
				sbits["pos"] = bits
				setTypes(sbits, sbits["pos"])
				tpl["getbuf"] = template(sbits, "sig_getbuf.tpl")
			}
			while (bits > 0) {
				delete sbits
				sbits["sign"] = "+"
				sbits["byte"] = int(bpos / 8)
				slice = bpos % 8 + 1
				if (slice > bits) {
					slice = bits
				}
				sbits["align"] = bpos % 8 + 1 - slice
				sbits["mask"] = sprintf("%#04x", 2^slice - 1)
				sbits["msk"] = 2^slice - 1
				sbits["#mask"] = "msk"
				sbits["pos"] = bits - slice
				setTypes(sbits, sbits["pos"] + slice - 1)
				tpl["getbuf"] = tpl["getbuf"] \
				                template(sbits, "sig_getbuf.tpl")
				tpl["setbuf"] = tpl["setbuf"] \
				                template(sbits, "sig_setbuf.tpl")
				bpos = (sbits["byte"] + 1) * 8 + 7
				bits -= slice
			}
		}
		# Load template
		printf("%s", template(tpl, "sig.tpl"))

		# Check for value table
		if (!(sig in obj_sig_enum)) {
			continue
		}

		# Load template
		printf("%s", template(tpl, "sig_enum.tpl"))

		# Print values
		i = 0
		while ((sig SUBSEP i) in obj_sig_enum_val) {
			tpl["enumval"] = obj_sig_enum_val[sig, i]
			tpl["enumname"] = obj_sig_enum_name[sig, i]
			tpl["comment"] = obj_sig_enum_desc[sig, i++]

			# Load template
			printf("%s", template(tpl, "sig_enumval.tpl"))
		}
	}

	# Introduce timeouts
	for (rel in obj_rel_attr) {
		if (obj_rel_attr_name[rel] != aTIMEOUT) {
			# Not a timeout
			continue
		}
		if (obj_attr_from[aTIMEOUT] != "ecu") {
			warn(aTIMEOUT " attribute discarded, because it is not a \"Node - Mapped RX Signal\" (ECU to signal) relation attribute")
			continue
		}
		if (obj_attr_to[aTIMEOUT] != "sig") {
			warn(aTIMEOUT " attribute discarded, because it is not a \"Node - Mapped RX Signal\" (ECU to signal) relation attribute")
			continue
		}
		delete tpl
		tpl["ecu"] = obj_rel_attr_from[rel]
		tpl["sig"] = obj_sig_name[obj_rel_attr_to[rel]]
		tpl["sigid"] = sigident(obj_rel_attr_to[rel])
		tpl["timeout"] = obj_rel_attr[rel]
		tpl["value"] = tpl["timeout"]   # For compatibility with
		tpl["#value"] = "timeout"       # older 3rd party templates
		tpl["msgid"] = sprintf("%x", msgid(obj_sig_msgid[obj_rel_attr_to[rel]]))
		tpl["msg"] = msgid(obj_sig_msgid[obj_rel_attr_to[rel]])
		tpl["#msgid"] = "msg"
		tpl["msgname"] = obj_msg_name[obj_sig_msgid[obj_rel_attr_to[rel]]]

		# Load template
		printf("%s", template(tpl, "timeout.tpl"))
	}

	# List enum
	for (enum in obj_enum) {
		delete tpl
		tpl["enum"] = enum
		tpl["db"] = obj_enum_db[enum]
		sub(/.*\//, "", tpl["db"])
		sub(/\.[^\.]*$/, "", tpl["db"])

		# Load template
		printf("%s", template(tpl, "enum.tpl"))

		# Print values
		i = 0
		while ((enum SUBSEP i) in obj_enum_val) {
			tpl["val"] = obj_enum_val[enum, i]
			tpl["name"] = obj_enum_name[enum, i]
			tpl["comment"] = obj_enum_desc[enum, i++]

			# Load template
			printf("%s", template(tpl, "enumval.tpl"))
		}
	}

}

