#!/usr/bin/awk -f

BEGIN {
	DEBUG = ENVIRON["DEBUG"]

	# Regexes for different types of data
	rLF = "\n"
	rFLOAT = "-?[0-9]+(\\.[0-9]+)?"
	rINT = "-?[0-9]+"
	rID = "[0-9]+"
	rDLC = "[0-8]"
	rSEP = "[:;,]"
	rSYM = "[[:alnum:]_]+"
	rSYMS = "(" rSYM ",)*" rSYM
	rSIG = "[0-9]+\\|[0-9]+@[0-9]+[-+]"
	rVEC = "\\(" rFLOAT "," rFLOAT "\\)"
	rBND = "\\[" rFLOAT "\\|" rFLOAT "\\]"
	#rSTR = "\"([^\"]|\\.)*\"" # That would be right if CANdb++ supported
	                           # escaping characters
	rSTR = "\"[^\"]*\""

	# Type strings
	tDISCARD["NS_"]
	tDISCARD["BS_"]
	tENUM["VAL_TABLE_"]
	tENUM["VAL_"]
	tVER = "VERSION"
	tECU = "BU_"
	tMSG = "BO_"
	tSIG = "SG_"
	tENV = "EV_"
	tCOM = "CM_"
	tATTRDEFAULT = "BA_DEF_DEF_"
	tATTRRANGE = "BA_DEF_"
	tATTR = "BA_"

	# Attribute types
	atSTR = "STRING"
	atENUM = "ENUM"
	atINT = "INT"

	# Environment variable types
	etINT = "INT"
	etFLOAT = "FLOAT"
	etDATA = "DATA"
	eTYPE[0] = etINT
	eTYPE[1] = etFLOAT
	eTYPE[2] = etDATA

	# Prominent attributes
	aSTART = "GenSigStartValue"
	aFCYCLE = "GenMsgCycleTimeFast"
	aCYCLE = "GenMsgCycleTime"
	aDELAY = "GenMsgDelayTime"
	aSEND = "GenMsgSendType"
}

{
	gsub(/[\r\n]*$/, "\n")
}

function buffer() {
	sub(/^[	 ]*/, "")
	if (!$0) {
		getline
		gsub(/[\r\n]*$/, "\n")
		sub(/^[	 ]*/, "")
	}
}

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

function fetch(types,
	str, re) {
	buffer()
	if (match($0, "^(" types ")")) {
		str = substr($0, 0, RLENGTH)
		# Cut str from the beginning of $0
		re = str
		gsub(/./, ".", re)
		sub(re, "")
	}
	if (DEBUG > 1 && str !~ /^[[:space:]]*$/) {
		print "dbc2c.awk: fetch: " str > "/dev/stderr"
	}
	return str
}

# Returns the expresion with ^ and $ at beginning and end to make ~ match
# entire expressions only.
function whole(re) {
	return "^" re "$"
}

# Remove quotes and escapes from strings.
function strip(str) {
	sub(/^"/, "", str)
	sub(/"$/, "", str)
	# CANdb++ does not allow " in strings, so there is no need for
	# handling escapes
	return str
}

function fsm_discard() {
	fetch(":")
	fetch(rLF)
	while(fetch(rSYM "|" rLF) !~ whole(rLF)) {
		fetch(rLF)
	}
}

# BU_
# * obj_ecu[ecu]
function fsm_ecu(dummy,
	ecu) {
	fetch(":")
	ecu = fetch(rSYM "|" rLF)
	while (ecu !~ whole(rLF)) {
		obj_ecu[ecu]
		ecu = fetch(rSYM "|" rLF)
	}
}

# VAL_TABLE_
# 1 obj_enum[enum]
# * obj_enum_entry[key] = val
#   obj_enum_table[key] = enum
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

# EV_
# 1 obj_env[name] = val
#   obj_env_type[name] = ("INT"|"FLOAT"|"DATA")
#   obj_env_min[name]
#   obj_env_max[name]
#   obj_env_unit[name] = (string)
function fsm_env(dummy,
	name, a) {
	name = fetch(rSYM)
	fetch(":")
	obj_env_type[name] = eTYPE[fetch(rID)]
	split(fetch(rBND), a, /[][|]/)
	obj_env_min[name] = a[2]
	obj_env_max[name] = a[3]
	obj_env_unit[name] = fetchStr()
	if (obj_env_type[name] == etINT) {
		obj_env[name] = int(fetch(rFLOAT))
	} else {
		obj_env[name] = fetch(rFLOAT)
	}
	if (DEBUG) {
		print "dbc2c.awk: obj_env[" name "] = " obj_env[name] > "/dev/stderr"
	}

	fetch(rID)  # Just a counter of environment variables
	fetch(rSYM) # DUMMY_NODE_VECTOR0
	fetch(rSYM) # Vector__XXX
	fetch(";")
}

# BO_
# 1 obj_msg[id]
#   obj_msg_name[id] = name
#   obj_msg_dlc[id] = dlc
#   obj_msg_tx[id] = ecu
#   obj_ecu_tx[ecu, i] = id
function fsm_msg(dummy,
	id,
	name,
	dlc,
	ecu, i) {
	id = fetch(rID)
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

	while (fetch(rSYM) == tSIG) {
		fsm_sig(id)
	}
}

# SG_
# 1 obj_sig[name]
#   obj_sig_msgid[name] = msgid
#   obj_sig_multiplexor[name] = (bool)
#   obj_sig_multiplexed[name] = (int)
#   obj_sig_sbit[name] = (uint)
#   obj_sig_len[name] = (uint)
#   obj_sig_intel[name] = (bool)
#   obj_sig_signed[name] = (bool)
#   obj_sig_mul[name] = (real)
#   obj_sig_off[name] = (real)
#   obj_sig_min[name] = (real)
#   obj_sig_max[name] = (real)
#   obj_sig_unit[name] = (string)
# * obj_sig_rx[name, i] = ecu
# * obj_ecu_rx[ecu, p] = name
function fsm_sig(msgid,
	name,
	multiplexing,
	a,
	ecu, i, p) {

	name = fetch(rSYM)
	obj_sig[name]
	obj_sig_msgid[name] = msgid
	multiplexing = fetch("m[0-9]+|M")
	obj_sig_multiplexor[name] = (multiplexing == "M")
	gsub(/[mM]/, "", multiplexing)
	obj_sig_miltiplexed[name] = multiplexing
	fetch(":")
	split(fetch(rSIG), a, /[|@]/)
	obj_sig_sbit[name] = a[1]
	obj_sig_len[name] = a[2]
	obj_sig_intel[name] = (a[3] ~ /^1/)
	obj_sig_signed[name] = (a[3] ~ /-$/)
	split(fetch(rVEC), a, /[(),]/)
	obj_sig_mul[name] = a[2]
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

# CM_
# 1 obj_db_comment[FILENAME]
# 1 obj_ecu_comment[name]
# 1 obj_env_comment[name]
# 1 obj_msg_comment[msgid]
# 1 obj_sig_comment[name]
function fsm_comment(dummy,
	context, name, msgid, str) {
	context = fetch(rSYM)
	if (context == "") {
		obj_db_comment[FILENAME] = fetchStr()
	} else if (context == tENV) {
		name = fetch(rSYM)
		obj_env_comment[name] = fetchStr()
	} else if (context == tECU) {
		name = fetch(rSYM)
		obj_ecu_comment[name] = fetchStr()
	} else if (context == tMSG) {
		msgid = fetch(rID)
		obj_msg_comment[msgid] = fetchStr()
	} else if (context == tSIG) {
		msgid = fetch(rID)
		name = fetch(rSYM)
		obj_sig_comment[name] = fetchStr()
	}
	fetch(";")
	fetch(rLF)
}

# BA_DEF
# 1 obj_attr[name]
#   obj_attr_type[name] = (INT|ENUM|STRING)
#   obj_attr_min[name]
#   obj_attr_max[name]
#   obj_attr_enum[name, i]
#   obj_attr_str[name]
function fsm_attrrange(dummy,
	context,
	name,
	type,
	val,
	i) {
	context = fetch(rSYM)
	if (context == tSIG) {
		context = "sig"
	} else if (context == tMSG) {
		context = "msg"
	} else {
		context = "db"
	}
	name = fetchStr()
	obj_attr_context[name] = context
	obj_attr[name]
	type = fetch(rSYM)
	obj_attr_type[name] = type
	if (type == atINT) {
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
	#TODO float, hex
	fetch(";")
}

# BA_DEF_DEF_
# 1 obj_attr_default[name] = value
# * obj_msg_attr[msgid, name]
# * obj_sig_attr[signame, name]
# * obj_db_attr[FILENAME, name]
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
	} else {
		obj_attr_default[name] = fetch(rFLOAT)
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

# BA_
# 1 obj_msg_attr[msgid, name]
# 1 obj_sig_attr[signame, name]
# 1 obj_db_attr[FILENAME, name]
function fsm_attr(dummy,
	name,
	id) {
	name = fetchStr()
	if (obj_attr_context[name] == "msg") {
		fetch(rSYM)
		id = fetch(rID)
		obj_msg_attr[id, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_msg_attr[" id "," name "] = " obj_msg_attr[id, name] > "/dev/stderr"
		}
	} else if (obj_attr_context[name] == "sig") {
		fetch(rSYM)
		fetch(rID)
		id = fetch(rSYM)
		obj_sig_attr[id, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_sig_attr[" id "," name "] = " obj_sig_attr[id, name] > "/dev/stderr"
		}
	} else if (obj_attr_context[name] == "db") {
		obj_db_attr[FILENAME, name] = fetch_attrval(name)
		if (DEBUG) {
			print "dbc2c.awk: obj_db_attr[" FILENAME "," name "] = " obj_db_attr[FILENAME, name] > "/dev/stderr"
		}
	}
	fetch(";")
}

# 1 obj_db[FILENAME]
function fsm_start(dummy,
	sym) {
	sym = fetch(rSYM "|" rLF)

	obj_db[FILENAME]

	# Discard version
	if (sym == tVER) {
		fetchStr()
	}
	# Discard not required symbols
	else if (sym in tDISCARD) {
		fsm_discard()
	}
	# Environment variables
	else if (sym == tENV) {
		fsm_env()
	}
	# Get ECUs
	else if (sym == tECU) {
		fsm_ecu()
	}
	# Get enums
	else if (sym in tENUM) {
		fsm_enum()
	}
	# Get message objects
	else if (sym == tMSG) {
		fsm_msg()
	}
	# Get comments
	else if (sym == tCOM) {
		fsm_comment()
	}
	# Get attribute ranges
	else if (sym == tATTRRANGE) {
		fsm_attrrange()
	}
	# Get attribute defaults
	else if (sym == tATTRDEFAULT) {
		fsm_attrdefault()
	}
	# Get attributes
	else if (sym == tATTR) {
		fsm_attr()
	}
}

{
	while ($0) {
		fsm_start()
	}
}

END {
}

