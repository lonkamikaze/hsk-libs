#!/usr/bin/awk -f

BEGIN {
	RS=""

	# Regexes for different types of data
	rLF = "\n"
	rNUM = "-?[0-9]+(\\.[0-9]+)?"
	rID = "[0-9]+"
	rSEP = "[:;,]"
	rSYM = "[[:alnum:]_]+"
	rSTR = "\"([^\"]|\\.)*\""
	rSIG = "[0-9]+\\|[0-9]+@[0-9]+[-+]"
	rVEC = "\\(" rNUM "," rNUM "\\)"
	rBND = "\\[" rNUM "\\|" rNUM "\\]"

	# Type strings
	tDISCARD["NS_"]
	tDISCARD["BS_"]
	tSKIP["EV_"]     # Environment variables
	tENUM["VAL_TABLE_"]
	tENUM["VAL_"]
	tVER = "VERSION"
	tECU = "BU_"
	tMSG = "BO_"
	tSIG = "SG_"
	tCOM = "CM_"
	tATTRDEFAULT = "BA_DEF_DEF_"
	tATTRRANGE = "BA_DEF_"
	tATTR = "BA_"

	# Attribute types
	atSTR = "STRING"
	atENUM = "ENUM"
	atINT = "INT"

	# Prominent attributes
	aSTART = "GenSigStartValue"
	aFCYCLE = "GenMsgCycleTimeFast"
	aCYCLE = "GenMsgCycleTime"
	aDELAY = "GenMsgDelayTime"
	aSEND = "GenMsgSendType"
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
	gsub(/\\(.)/, "&", str) # Does that work?
	return str
}

function fetch(types,
	str, re) {
	sub(/^[	 ]*/, "")
	if (match($0, "^(" types ")")) {
		str = substr($0, 0, RLENGTH)
		# Cut str from the beginning of $0
		re = str
		gsub(/./, ".", re)
		sub(re, "")
	}
	return str
}

function fsm_discard() {
	fetch(":")
	fetch(rLF)
	while(fetch(rSYM "|" rLF) !~ whole(rLF)) {
		fetch(rLF)
	}
}

function fsm_skip() {
	while(fetch(rSYM "|" rSEP "|" rID "|" rBND "|" rSTR "|" rNUM) !~ whole(";"));
	fetch(rLF)
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
	val = fetch(rNUM "|;")
	while (val !~ whole(";")) {
		key = enum "_" strip(fetch(rSTR))
		obj_enum_table[key] = enum
		obj_enum_entry[key] = val
		val = fetch(rNUM "|;")
	}
	fetch(rLF)
}

# BO_
# 1 obj_msg[id]
#   obj_msg_name[id] = name
#   obj_msg_dlc[id] = dlc
function fsm_msg(dummy,
	id,
	name,
	dlc) {
	id = fetch(rID)
	name = fetch(rSYM)
	fetch(":")
	dlc = fetch("[0-8]")
	fetch(rSYM)
	fetch(rLF)

	obj_msg[id]
	obj_msg_name[id] = name
	obj_msg_dlc[id] = dlc

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
function fsm_sig(msgid,
	name,
	multiplexing,
	a) {

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
	obj_sig_unit[name] = strip(fetch(rSTR))
	fetch(rSYM)
	fetch(rLF)
}

# CM_
# 1 obj_db_comment[FILENAME]
# 1 obj_ecu_comment[name]
# 1 obj_msg_comment[msgid]
# 1 obj_sig_comment[name]
function fsm_comment(dummy,
	context, name, msgid, str) {
	context = fetch(rSYM)
	if (context == "") {
		obj_db_comment[FILENAME] = strip(fetch(rSTR))
	} else if (context in tSKIP) {
		fsm_skip()
	} else if (context == tECU) {
		name = fetch(rSYM)
		obj_ecu_comment[name] = strip(fetch(rSTR))
	} else if (context == tMSG) {
		msgid = fetch(rID)
		obj_msg_comment[msgid] = strip(fetch(rSTR))
	} else if (context == tSIG) {
		msgid = fetch(rID)
		name = fetch(rSYM)
		obj_sig_comment[name] = strip(fetch(rSTR))
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
	name = strip(fetch(rSTR))
	obj_attr_context[name] = context
	obj_attr[name]
	type = fetch(rSYM)
	obj_attr_type[name] = type
	if (type == atINT) {
		obj_attr_min[name] = fetch(rNUM)
		obj_attr_max[name] = fetch(rNUM)
	} else if (type == atENUM) {
		val = strip(fetch(rSTR))
		while (val != "") {
			obj_attr_enum[name, ++i] = val
			fetch(",")
			val = strip(fetch(rSTR))
		}
	} else if (type == atSTR) {
		obj_attr_str[name] = strip(fetch(rSTR))
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
	name = strip(fetch(rSTR))
	if (obj_attr_type[name] == atSTR) {
		obj_attr_default[name] = strip(fetch(rSTR))
	} else if (obj_attr_type[name] == atENUM) {
		value = strip(fetch(rSTR))
		while (obj_attr_enum[name, ++i] != value);
		obj_attr_default[name] = i
	} else {
		obj_attr_default[name] = fetch(rNUM)
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
	if (obj_attr_type[attribute] == "STRING") {
		return strip(fetch(rSTR))
	}
	return fetch(rNUM)
}

# BA_
# 1 obj_msg_attr[msgid, name]
# 1 obj_sig_attr[signame, name]
# 1 obj_db_attr[FILENAME, name]
function fsm_attr(dummy,
	name,
	id) {
	name = strip(fetch(rSTR))
	if (obj_attr_context[name] == "msg") {
		for (i in obj_msg) {
			obj_msg_attr[i, name] = obj_attr_default[name]
		}
		fetch(rSYM)
		id = fetch(rID)
		obj_msg_attr[id, name] = fetch_attrval(name)
	} else if (obj_attr_context[name] == "sig") {
		fetch(rSYM)
		fetch(rID)
		id = fetch(rSYM)
		obj_sig_attr[id, name] = fetch_attrval(name)
	} else if (obj_attr_context[name] == "db") {
		obj_db_attr[FILENAME, name] = fetch_attrval(name)
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
		fetch(rSTR)
	}
	# Discard not required symbols
	else if (sym in tDISCARD) {
		fsm_discard()
	}
	# Skip to semicolon
	else if (sym in tSKIP) {
		fsm_skip()
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
	gsub(/\r/, "")
	while ($0) {
		fsm_start()
	}
}

