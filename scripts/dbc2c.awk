#!/usr/bin/awk -f
#
# This script parses Vector CAN DBs (.dbc files), such as can be created
# using Vector CANdb++.
#
# A subset of the parsed information is output using a set of templates.
#
# @note
#	Note, pipe the input through "iconv -f CP1252" so GNU AWK doesn't choke
#	on non-UTF-8 characters in comments.
#

#
# Initialises globals.
#
BEGIN {
	# Environment variables
	DEBUG = ENVIRON["DEBUG"]
	TEMPLATES = ENVIRON["TEMPLATES"]
	DATE = ENVIRON["DATE"]

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
	rDLC = "[0-8]"
	rSEP = "[:;,]"
	rSYM = "[a-zA-Z0-9_]+"
	rSYMS = "(" rSYM ",)*" rSYM
	rSIG = "[0-9]+\\|[0-9]+@[0-9]+[-+]"
	rVEC = "\\(" rFLOAT "," rFLOAT "\\)"
	rBND = "\\[" rFLOAT "\\|" rFLOAT "\\]"
	#rSTR = "\"([^\"]|\\.)*\"" # That would be right if CANdb++ supported
	                           # escaping characters
	rSTR = "\"[^\"]*\""

	# Type strings
	tDISCARD["BS_"]
	tENUM["VAL_TABLE_"]
	tENUM["VAL_"]
	t["ENUM"] = "VAL_"
	t["ENUM_DEF"] = "VAL_TABLE_"
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

#
# Strip DOS line endings and make sure there is a new line symbol at the
# end of the line, so multiline definitions can be parsed.
#
{
	gsub(/[\r\n]*$/, "\n")
}

#
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

#
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

#
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
		re = str
		gsub(/./, ".", re)
		sub(re, "")
	}
	if (DEBUG > 1 && str !~ /^[ \t\n]*$/) {
		print "dbc2c.awk: fetch: " str > "/dev/stderr"
	}
	if (str) {
		fetch_loop_detect = 0
	} else if (fetch_loop_detect++ >= 100) {
		print "dbc2c.awk: ERROR infinite loop detected!" > "/dev/stderr"
		errno = 11
		exit
	}
	return str
}

#
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

#
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
	# CANdb++ does not allow " in strings, so there is no need for
	# handling escapes
	return str
}

#
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
	print "dbc2c.awk: ERROR unknown context " str " encountered" > "/dev/stderr"
	errno = 2
	exit
}

#
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

#
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

#
# Parse a value table.
#
# Token: VAL_, VAL_TABLE_
#
# Creates:
# - 1 obj_enum[enum]
# - * obj_enum_entry[key] = val
# - * obj_enum_table[key] = enum
#
function fsm_enum(dummy,
	enum,
	val,
	key) {
	fetch(rID)
	enum = fetch(rSYM)
	obj_enum_[enum]
	val = fetch(rINT "|;")
	while (val !~ whole(";")) {
		key = enum "_" fetchStr()
		obj_enum_table[key] = enum
		obj_enum_entry[key] = val
		val = fetch(rINT "|;")
	}
	fetch(rLF)
}

#
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
	if (DEBUG) {
		print "dbc2c.awk: obj_env[" name "]" > "/dev/stderr"
	}
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
		print "dbc2c.awk: Environment variable type '" obj_env_type[name] "' not implemented!" > "/dev/stderr"
		errno = 3
		exit
	}

	fetch(rID)  # Just a counter of environment variables
	fetch(rSYM) # DUMMY_NODE_VECTOR0
	fetch(rSYM) # Vector__XXX
	fetch(";")
}

#
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
	if (DEBUG) {
		print "dbc2c.awk: obj_env_dlc[" name "]" > "/dev/stderr"
	}
	fetch(":")
	obj_env_dlc[name] = fetch(rID)
	fetch(";")
}

#
# Parse a message definition.
#
# Token: BO_
#
# Creates:
# - 1 obj_msg[id]
# - 1 obj_msg_ext[id] = extended
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
	ext, i) {
	id = fetch(rID)
	ext = (int(id / 2^31) == 1)
	if (ext) {
		id -= (2^31)
	}
	if (DEBUG) {
		print "dbc2c.awk: obj_msg[" (ext ? "X." : "") id "(" sprintf("%#x", id) ")]" > "/dev/stderr"
	}
	obj_msg_ext[id] = ext
	name = fetch(rSYM)
	fetch(":")
	dlc = fetch(rDLC)
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

#
# Parse a signature definition.
#
# Token: SG_
#
# Creates:
# - 1 obj_sig[name]
# - 1 obj_sig_msgid[name] = msgid
# - 1 obj_sig_multiplexor[name] = (bool)
# - 1 obj_sig_multiplexed[name] = (int)
# - 1 obj_sig_sbit[name] = (uint)
# - 1 obj_sig_len[name] = (uint)
# - 1 obj_sig_intel[name] = (bool)
# - 1 obj_sig_signed[name] = (bool)
# - 1 obj_sig_fac[name] = (float)
# - 1 obj_sig_off[name] = (float)
# - 1 obj_sig_min[name] = (float)
# - 1 obj_sig_max[name] = (float)
# - 1 obj_sig_unit[name] = (string)
# - * obj_sig_rx[name, i] = ecu
# - * obj_ecu_rx[ecu, p] = name
# - * obj_msg_sig[msgid, p] = name
#
function fsm_sig(msgid,
	name,
	multiplexing,
	a,
	ecu, i, p) {
	name = fetch(rSYM)
	if (DEBUG) {
		print "dbc2c.awk: obj_sig[" name "]" > "/dev/stderr"
	}
	obj_sig[name]
	obj_sig_msgid[name] = msgid
	while (obj_msg_sig[msgid, p++]);
	obj_msg_sig[msgid, --p] = name
	multiplexing = fetch("m[0-9]+|M")
	obj_sig_multiplexor[name] = (multiplexing == "M")
	gsub(/[mM]/, "", multiplexing)
	obj_sig_multiplexed[name] = multiplexing
	fetch(":")
	split(fetch(rSIG), a, /[|@]/)
	obj_sig_sbit[name] = a[1]
	obj_sig_len[name] = a[2]
	obj_sig_intel[name] = (a[3] ~ /^1/)
	obj_sig_signed[name] = (a[3] ~ /-$/)
	split(fetch(rVEC), a, /[(),]/)
	obj_sig_fac[name] = a[2]
	obj_sig_off[name] = a[3]
	split(fetch(rBND), a, /[][|]/)
	obj_sig_min[name] = a[2]
	obj_sig_max[name] = a[3]
	obj_sig_unit[name] = fetchStr()
	split(fetch(rSYMS), a, /,/)
	for (ecu in a) {
		if (a[ecu] in obj_ecu) {
			obj_sig_rx[name, i++] = a[ecu]
			p = 0
			while (obj_ecu_rx[a[ecu], p++]);
			obj_ecu_rx[a[ecu], --p] = name
		}
	}
	fetch(rLF)
}

#
# Parse comments.
#
# Token: CM_
#
# Creates one of:
# - 1 obj_db_comment[FILENAME]
# - 1 obj_ecu_comment[name]
# - 1 obj_env_comment[name]
# - 1 obj_msg_comment[msgid]
# - 1 obj_sig_comment[name]
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
		obj_sig_comment[name] = fetchStr()
	} else {
		print "dbc2c.awk: Comment for '" context "' not implemented!" > "/dev/stderr"
		errno = 10
		exit
	}
	fetch(";")
	fetch(rLF)
}

#
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

#
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

#
# Parse attribute default value.
#
# Token: BA_DEF_DEF_
#
# Creates:
# - 1 obj_attr_default[name] = value
# - * obj_msg_attr[msgid, name]
# - * obj_sig_attr[signame, name]
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
		print "dbc2c.awk: Attribute type '" obj_attr_type[name] "' not implemented!" > "/dev/stderr"
		errno = 4
		exit
	}
	if (DEBUG) {
		print "dbc2c.awk: obj_attr_default[" name "] = " obj_attr_default[name] > "/dev/stderr"
	}
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

#
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
		return fetch(rINT)
	} else if (obj_attr_type[attribute] == atINT) {
		return int(fetch(rFLOAT))
	}
	return fetch(rFLOAT)
}

#
# Parse an attribute value.
#
# Token: BA_
#
# Creates one of:
# - 1 obj_sig_attr[signame, name] = value
# - 1 obj_msg_attr[msgid, name] = value
# - 1 obj_ecu_attr[ecu, name] = value
# - 1 obj_db_attr[FILENAME, name] = value
#
function fsm_attr(dummy,
	name,
	id) {
	name = fetchStr()
	if (obj_attr_context[name] == "sig") {
		fetch(t["SIG"])
		fetch(rID) # Fetch message ID
		id = fetch(rSYM)
		obj_sig_attr[id, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_sig_attr[" id ", " name "] = " obj_sig_attr[id, name] > "/dev/stderr"
		}
	}
	else if (obj_attr_context[name] == "msg") {
		fetch(t["MSG"])
		id = fetch(rID)
		obj_msg_attr[id, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_msg_attr[" id "," name "] = " obj_msg_attr[id, name] > "/dev/stderr"
		}
	}
	else if (obj_attr_context[name] == "ecu") {
		fetch(t["ECU"])
		id = fetch(rSYM)
		obj_ecu_attr[id, name] = fetch_attrval(name)
	}
	else if (obj_attr_context[name] == "db") {
		obj_db_attr[FILENAME, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_db_attr[" FILENAME "," name "] = " obj_db_attr[FILENAME, name] > "/dev/stderr"
		}
	}
	else {
		print "dbc2c.awk: ERROR attributes for " obj_attr_context[name] " not implemented!" > "/dev/stderr"
		errno = 6
		exit
	}
	fetch(";")
}

#
# Parse a relation attribute value.
#
# Token: BA_REL_
#
# Creates:
# - 1 obj_rel_attr[from, to, name] = value
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
		fetch(rID) # Fetch message ID
		from = fetch(rSYM)
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
		print "dbc2c.awk: ERROR relation attributes for " obj_attr_from[name] " not implemented!" > "/dev/stderr"
		errno = 7
		exit
	}
	fetch(rSYM) # Fetch the type of the related object
	if (obj_attr_to[name] == "sig") {
		fetch(rID) # Fetch message ID
		to = fetch(rSYM)
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
		print "dbc2c.awk: ERROR relation attributes for " obj_attr_to[name] " not implemented!" > "/dev/stderr"
		errno = 8
		exit
	}
	obj_rel_attr[from, to, name] = fetch_attrval(name)
	if (DEBUG) {
		print "dbc2c.awk: obj_rel_attr[" from ", " to ", " name "] = " obj_rel_attr[from, to, name] > "/dev/stderr"
	}
	fetch(";")
}

#
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
			print "dbc2c.awk: ERROR unknown symbol " sym " encountered" > "/dev/stderr"
			errno = 5
			exit
		}
	}
}

#
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

#
# Gets a signal group.
#
# Token: SIG_GROUP_
#
# Creates:
# - 1 obj_siggrp[id] = name
# - 1 obj_siggrp_db[id] = FILENAME
# - * obj_siggrp_sig[id, i] = sig
# - * obj_sig_grp[sig, p] = id
#
function fsm_siggrp(dummy,
	id, i,
	sig, p) {
	id = fetch(rID)
	obj_siggrp[id] = fetch(rSYM)
	if (DEBUG) {
		print "dbc2c.awk: obj_siggrp[" id "] = " obj_siggrp[id] > "/dev/stderr"
	}
	obj_siggrp_db[id] = FILENAME
	fetch(rID) # Meaning unknown!
	fetch(":")
	i = 0
	while (!fetch(";")) {
		sig = fetch(rSYM)
		obj_siggrp_sig[id, i++] = sig
		p = 0
		while (obj_sig_grp[sig, p++]);
		obj_sig_grp[sig, --p] = id
	}
}

#
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
		print "dbc2c.awk: ERROR unsupported symbol " sym " encountered" > "/dev/stderr"
		errno = 9
		exit
	}
	# Check known symbols
	if (sym == t["SYMBOLS"]) {
		fsm_symbols()
	}
	# Discard version
	else if (sym == tVER) {
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
	# Get enums
	else if (sym in tENUM) {
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

#
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

#
# Returns a string contining all elements of an array connected by a
# seperator.
#
# @param sep
#	The seperator to use between concatenated strings
# @param array
#	The array to concatenate
#
function join(sep, array,
	i, result, a) {
	for (i in array) {
		result = result (a++ ? sep : "") array[i]
	}
	return result
}

#
# Returns a string contining all indexes of an array connected by a
# seperator.
#
# @param sep
#	The seperator to use between concatenated strings
# @param array
#	The array to concatenate
#
function joinIndex(sep, array,
	i, result, a) {
	for (i in array) {
		result = result (a++ ? sep : "") i
	}
	return result
}

#
# Returns the greatest common divider (GCD).
#
# @param a
#	An integer
# @param b
#	An integer
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

#
# Returns a compact string representation of a rational number.
#
# @param n
#	The numerator
# @param d
#	The denominator
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

#
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

#
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

#
# Returns a rational string representation of a real value.
#
# This uses the different rational*() functions to find a minimal
# representation of the value.
#
# @param val
#	The real value to return as a rational
# @param precision
#	The maximum number of bits for either rational component
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

#
# Populates a template line with data.
#
# Multiline data in a template needs to be in its own line.
#
# Lines with empty data fields are removed.
#
# Identifiers in templates have the following shape:
#	"<:" name ":>"
#
# @param data
#	The array containing field data
# @param line
#	The line to perform substitutions in
# @return
#	The line(s) with performed substitutions
#
function tpl_line(data, line,
	name, pre, post, count, array, i) {
	while(match(line, /<:[a-zA-Z0-9]+:>/)) {
		name = substr(line, RSTART + 2, RLENGTH - 4)
		pre = substr(line, 1, RSTART - 1)
		post = substr(line, RSTART + RLENGTH)
		count = split(data[name], array, RS)
		if (!count) {
			line = ""
			return
		}
		line = pre array[1] post
		for (i = 2; i <= count; i++) {
			line = line ORS pre array[i] post
		}
	}
	return line ORS
}

#
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
	buf = ""
	while (getline line < name) {
		buf = buf tpl_line(data, line)
	}
	close(name)
	return buf
}

#
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
		while (obj_ecu_tx[ecu, p]) {
			tx[sprintf("%x", obj_ecu_tx[ecu, p++])]
		}
		tpl["tx"] = joinIndex(RS, tx)
		# RX signals
		p = 0
		delete rx
		while (obj_ecu_rx[ecu, p]) {
			rx[obj_ecu_rx[ecu, p++]]
		}
		tpl["rx"] = joinIndex(RS, rx)
		# Load template
		printf("%s", template(tpl, "ecu.tpl"))
	}

	# Introduce signal groups
	for (id in obj_siggrp) {
		delete tpl
		tpl["id"] = id
		tpl["name"] = obj_siggrp[id]
		# DB
		tpl["db"] = obj_siggrp_db[id]
		sub(/.*\//, "", tpl["db"])
		sub(/\.[^\.]*$/, "", tpl["db"])
		# Signals
		delete sigs
		p = 0
		while (obj_siggrp_sig[id, p]) {
			sigs[obj_siggrp_sig[id, p++]]
		}
		tpl["sig"] = joinIndex(RS, sigs)
		# Load template
		printf("%s", template(tpl, "siggrp.tpl"))
	}

	# Introduce the Messages
	for (msg in obj_msg) {
		delete tpl
		tpl["id"] = sprintf("%x", msg)
		tpl["name"] = obj_msg_name[msg]
		tpl["comment"] = obj_msg_comment[msg]
		tpl["ecu"] = obj_msg_tx[msg]
		tpl["ext"] = obj_msg_ext[msg]
		tpl["dlc"] = obj_msg_dlc[msg]
		tpl["cycle"] = obj_msg_attr[msg, aCYCLE]
		tpl["fast"] = obj_msg_attr[msg, aFCYCLE]
		# Get signal list
		i = 0
		delete sigs
		while (obj_msg_sig[msg, i]) {
			sigs[obj_msg_sig[msg, i++]]
		}
		tpl["sig"] = joinIndex(RS, sigs)
		# Load template
		printf("%s", template(tpl, "msg.tpl"))
	}

	# Introduce the Signals
	for (sig in obj_sig) {
		delete tpl
		tpl["name"] = sig
		tpl["comment"] = obj_sig_comment[sig]
		# Reference the message this signal belongs to
		tpl["msgid"] = sprintf("%x", obj_sig_msgid[sig])
		# Reference the signal groups this message belongs to
		i = 0
		delete grps
		while (obj_sig_grp[sig, i]) {
			grps[obj_sig_grp[sig, i++]]
		}
		tpl["sgid"] = joinIndex(RS, grps)
		# Reference the RX ECUs
		i = 0
		delete ecus
		while (obj_sig_rx[sig, i]) {
			ecus[obj_sig_rx[sig, i++]]
		}
		tpl["ecu"] = joinIndex(RS, ecus)
		# Tuple
		tpl["endian"] = obj_sig_intel[sig] ? "INTEL" : "MOTOROLA"
		tpl["signed"] = obj_sig_signed[sig]
		tpl["sbit"] = obj_sig_sbit[sig]
		tpl["len"] = obj_sig_len[sig]
		# Setup/Start
		tpl["start"] = obj_sig_attr[sig, aSTART]
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
			while (bits > 0) {
				delete sbits
				sbits["byte"] = int(bpos / 8)
				sbits["align"] = bpos % 8
				shift = 8 - sbits["align"]
				if (shift > bits) {
					shift = bits
				}
				sbits["mask"] = sprintf("%#04x", 2^shift - 1)
				sbits["pos"] = pos
				tpl["getbuf"] = tpl["getbuf"] \
				                template(sbits, "sig_getbuf.tpl")
				sub(ORS "$", "", tpl["getbuf"])
				tpl["setbuf"] = tpl["setbuf"] \
				                template(sbits, "sig_setbuf.tpl")
				sub(ORS "$", "", tpl["setbuf"])
				pos += shift
				bpos += shift
				bits -= shift
			}
		} else {
			# Motorola signals
			tpl["getbuf"] = ""
			tpl["setbuf"] = ""
			while (bits > 0) {
				delete sbits
				sbits["byte"] = int(bpos / 8)
				slice = bpos % 8 + 1
				if (slice > bits) {
					slice = bits
				}
				sbits["align"] = bpos % 8 + 1 - slice
				sbits["mask"] = sprintf("%#04x", 2^slice - 1)
				sbits["pos"] = bits - slice
				tpl["getbuf"] = tpl["getbuf"] \
				                template(sbits, "sig_getbuf.tpl")
				sub(ORS "$", "", tpl["getbuf"])
				tpl["setbuf"] = tpl["setbuf"] \
				                template(sbits, "sig_setbuf.tpl")
				sub(ORS "$", "", tpl["setbuf"])
				bpos = (sbits["byte"] + 1) * 8 + 7
				bits -= slice
			}
		}
		# Load template
		printf("%s", template(tpl, "sig.tpl"))
	}

	# Introduce timeouts
	for (rel in obj_rel_attr) {
		delete tpl
		delete ids
		split(rel, ids, SUBSEP)
		if (ids[3] != aTIMEOUT) {
			continue
		}
		# That GenSigTimeoutTime is an ECU to SIG relation is
		# blindly assumed here, that might be dangerous!
		tpl["ecu"] = ids[1]
		tpl["sig"] = ids[2]
		tpl["value"] = obj_rel_attr[rel]
		# Load template
		printf("%s", template(tpl, "timeout.tpl"))
	}

}

