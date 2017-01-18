#!/usr/bin/awk -f
#
# Creates build and link instructions for a given project path.
#
# This script is a wrapper around `cpp -MM` and has a performance
# comparable to the `mkdep` tool.
#
# The easiest way to invoke the script is by calling:
#
#	scripts/build.awk src/
#
# This command outputs build instructions for all C and C++ files
# found in `src/`.
#
# It will commonly be necessary to add custom arguments to the CPP
# call, i.e. because a define, an include directory or a specific
# version of the C/C++ standard is required:
#
#	scripts/build.awk src/ -I/usr/local/include -DRELEASE -std=c++14
#
# These arguments have to come after the first source directory or
# the AWK interpreter will treat then as unknown arguments to itself
# instead of arguments to the script.
#
# The script supports building from multiple sources by providing
# multiple input directories. In case of multiple objects with the
# same name the first one encountered wins.
#
# Environment
# -----------
#
# A number of variables make it possible to steer the behaviour of
# build.awk, either by setting them in the environment or by providing
# them explicitly via `-vVAR=VALUE`:
#
# | Variable  | Description                            | Default          |
# |-----------|----------------------------------------|------------------|
# | `DEBUG`   | Set to enable verbose output           | `""`             |
# | `CPP`     | Change to pass flags to `cpp`          | `"cpp -MM %s"`   |
# | `FIND`    | Control `find` syntax                  | `"find -E %s …"` |
# | `MKDIR`   | Command for creating build directories | `"mkdir -p %s"`  |
# | `OBJSUFX` | Set to control suffix of object files  | `".o"`           |
# | `BINSUFX` | Set to control suffix of binaries      | `""`             |
#
# The string `" -MM %s"` is implicitly appended to `CPP` if it set
# through the environment.
#
# Special Make Variables
# ----------------------
#
# For better portability a set of make variables that evaluate to
# target local variables are defined. The variables have a default
# meaning and a BSD make specific fallback:
#
# | Variable  | Default | BSD Fallback     |
# |-----------|---------|------------------|
# | `_IMPSRC` | `$<`    | `${.ALLSRC:[1]}` |
# | `_TARGET` | `$@`    | `${.TARGET}`     |
# | `_ALLSRC` | `$^`    | `${.ALLSRC}`     |
#
# These can be overridden if needed.
#
# Build Flags
# -----------
#
# There are some options to influence the build without rerunning
# build.awk:
#
# | Variable     | Meaning                         | Default    |
# |--------------|---------------------------------|------------|
# | `CC`         | C compiler to use               | OS defined |
# | `CFLAGS`     | Flags to the C compiler         | OS defined |
# | `CXX`        | C++ compiler to use             | OS defined |
# | `CXXFLAGS`   | Flags to the C++ compiler       | OS defined |
# | `LDFLAGS`    | Linker flags                    | OS defined |
# | `LDLIBS`     | Libraries to link               | OS defined |
# | `OBJDIR`     | Optional build output directory | undefined  |
# | `COMPFLAGS*` | Additional flags for compiling  | undefined  |
# | `LINKFLAGS*` | Additional flags for linking    | undefined  |
# | `LINKLIBS*`  | Additional libraries to link    | undefined  |
#
# If defined the `OBJDIR` variable must terminate with a `/`.
#
# The `COMPFLAGS*`, `LINKFLAGS*` and `LINKLIBS*` variables are variable
# hierarchies. E.g. for the target `foo/bar.o` the following
# hierarchy of compile arguments will be created:
#
#	${COMPFLAGS} ${COMPFLAGS_foo} ${COMPFLAGS_foo_bar_o}
#
# Meta Targets
# ------------
#
# The output of this script provides two meta targets:
#
# | Target | Description        |
# |--------|--------------------|
# | build  | Build all binaries |
# | all    | Build everything   |
#

##
# This function handles all input and produces the output of the
# script.
#
# In addition to the user-definable globals it uses the following
# globals to keep state:
#
# | Variable   | Source                 | Description                    |
# |------------|------------------------|--------------------------------|
# | `SRCDIRS`  | Command line arguments | The list of source directories |
# | `CPPARGS`  | Command line arguments | Additional `CPP` arguments     |
# | `SRCS`     | `FIND` in `SRCDIRS`    | List of source files           |
# | `INCLUDES` | `CPP` output           | Map: object → full sources     |
# | `OBJSRC`   | `CPP` output           | Map: object → source           |
# | `COMPILER` | `CPP` output           | Map: object → compiler/flags   |
# | `ORIGIN`   | `CPP` output           | Map: origin → object           |
#
BEGIN {
	# set up debugging flag
	if (!DEBUG) {
		DEBUG = ENVIRON["DEBUG"]
	}
	verbose("DEBUG", DEBUG)
	# set up cpp command for dependencies
	if (!CPP && !(ENVIRON["CPP"] ? CPP = ENVIRON["CPP"] " -MM %s" : "")) {
		CPP = "cpp -MM %s"
	}
	verbose("CPP", CPP)
	# set up find command for source files
	if (!FIND && !(FIND = ENVIRON["FIND"])) {
		FIND = "find -E %s -regex '.*\\.(c|cc|cpp|cxx|C)$'"
	}
	verbose("FIND", FIND)
	# set up mkdir command
	if (!MKDIR && !(MKDIR = ENVIRON["MKDIR"])) {
		MKDIR = "mkdir -p %s"
	}
	verbose("MKDIR", MKDIR)
	# set up object suffix
	if (!OBJSUFX && !(OBJSUFX = ENVIRON["OBJSUFX"])) {
		OBJSUFX = ".o"
	}
	verbose("OBJSUFX", OBJSUFX)
	# set up binary suffix
	if (!BINSUFX && !(BINSUFX = ENVIRON["BINSUFX"])) {
		BINSUFX = ""
	}
	verbose("BINSUFX", BINSUFX)

	# process command line arguments
	delete SRCDIRS
	CPPARGS = ""
	p = 1
	for (i = 1; i < ARGC; ++i) {
		if (ARGV[i] ~ /^-/) {
			CPPARGS = CPPARGS " " ARGV[i]
		} else {
			sub(/\/?$/, "/", ARGV[i]) # Ensure terminal /
			SRCDIRS[p++] = ARGV[i]
		}
		delete ARGV[key]
	}
	verbose_values("SRCDIRS", SRCDIRS)
	verbose("CPPARGS", CPPARGS)

	# gmake/bsdmake compatible target local variables
	#       | var    | gmake
	#       |        | | bsdmake fallback
	printf("_IMPSRC?=$<${<:D:U${.ALLSRC:[1]}}\n") # first source
	printf("_TARGET?=$@${@:D:U${.TARGET}}\n")     # target
	printf("_ALLSRC?=$^${^:D:U${.ALLSRC}}\n")     # all sources

	delete INCLUDES # object → full include paths
	delete OBJSRC   # object → local source path
	delete COMPILER # object → compiler, flags
	delete ORIGIN   # full path without suffix → object

	# process all dirs
	for (i = 1; i in SRCDIRS; ++i) {
		dir = SRCDIRS[i]
		# get all source files
		delete SRCS
		cmd = sprintf(FIND, dir)
		while ((cmd | getline) > 0) {
			SRCS[$0]
		}
		close(cmd)
		verbose_keys("SRCS", SRCS)

		# get dependencies
		files = ""
		for (file in SRCS) {
			files = files " " file
		}
		cmd = sprintf(CPP, CPPARGS files)
		object = ""
		while ((cmd | getline) > 0) {
			if ($1 ~ /^[^:]*:$/) {
				$1 = ""
				objsrc = substr($2, length(dir) + 1)
				compiler = "false"
				if (objsrc ~ /\.(c|cc|C)$/) {
					compiler = "${CC} ${CFLAGS}"
				} else if (objsrc ~ /\.(cpp|cxx)$/) {
					compiler = "${CXX} ${CXXFLAGS}"
				}
				object = src_to_obj(objsrc)
				# in case of collision first source dir wins
				if (object in OBJSRC) {
					object = ""
					continue;
				}
				OBJSRC[object] = objsrc
				COMPILER[object] = compiler
				ORIGIN[src_to_origin($2)] = object
			}
			if (!object) { continue; }
			sub(/ \\$/, "")
			gsub(/  */, " ")
			$0 = compact($0)
			INCLUDES[object] = INCLUDES[object] $0
		}
		close(cmd)
	}

	# print build instructions
	for (object in INCLUDES) {
		# print compile instructions
		compflags = flags(object, " ${COMPFLAGS%s}")
		objdir = object
		gsub(/[^\/]*$/, "", objdir)
		if (objdir) {
			objdir = sprintf(MKDIR, "${OBJDIR}" objdir)
			objdir = sprintf("\t@%s\n", objdir)
		}
		printf("${OBJDIR}%s:%s\n%s\t%s%s -c ${_IMPSRC} -o ${_TARGET}\n",
		       object, INCLUDES[object], objdir,
		       COMPILER[object], compflags)
		all = all " ${OBJDIR}" object
	}

	# get linkable files
	delete LINKED
	for (i = 1; i in SRCDIRS; ++i) {
		dir = SRCDIRS[i]
		# grep for files containing an `int main(` or `void main(`
		cmd = "grep -IElr '(void|int)[ \\t\\n]*main[ \\t\\n]*\\(' " dir
		while ((cmd | getline) > 0) {
			# get the object file containing the main
			binsrc = substr($0, length(dir) + 1)
			binobj = src_to_obj(binsrc)
			bin = obj_to_bin(binobj)

			# skip on object name colission with previous dir
			if (bin in LINKED) { continue; }
			LINKED[bin]

			# get the object files to link
			linkobjs = linklist(binobj)
			gsub(/ /, " ${OBJDIR}", linkobjs)
			# get flags for object files
			linkflags = flags(bin, " ${LINKFLAGS%s}")
			linklibs = flags(bin, " ${LINKLIBS%s}")
			delete aobjs

			# print link instructions
			printf("${OBJDIR}%s:%s\n\t%s ${LDFLAGS}%s ${_ALLSRC} ${LDLIBS}%s -o ${_TARGET}\n",
			       bin, linkobjs, COMPILER[binobj], linkflags, linklibs)
			all = all " ${OBJDIR}" bin
			build = build " ${OBJDIR}" bin
		}
		close(cmd)
	}

	# print meta targets
	printf(".PHONY: all build\n")
	printf("all:%s\n", all)
	printf("build:%s\n", build)
}

##
# Print a name/value pair if `DEBUG` is set.
#
# @param name
#	The name to print
# @param value
#	The value to print
#
function verbose(name, value) {
	if (!DEBUG) { return }
	printf("build.awk: %s: %s\n", name, value) >"/dev/stderr"
}

##
# Print a name and the values of an array if `DEBUG` is set.
#
# Note this function assumes the array is indexed in 1 increments
# starting at 1.
#
# @param name
#	The name to print
# @param a
#	An array to print the values of
#
function verbose_values(name, a,
                        result, i) {
	if (!DEBUG) { return }
	for (i = 1; i in a; ++i) {
		result = (result ? result ", " : "[") a[i]
	}
	result = result (result ? "]" : "")
	printf("build.awk: %s: %s\n", name, result) >"/dev/stderr"
}


##
# Print a name and the keys of an array if `DEBUG` is set.
#
# @param name
#	The name to print
# @param a
#	An array to print the keys of
#
function verbose_keys(name, a,
                      result, key) {
	if (!DEBUG) { return }
	for (key in a) {
		result = (result ? result ", " : "[") key
	}
	result = result (result ? "]" : "")
	printf("build.awk: %s: %s\n", name, result) >"/dev/stderr"
}

##
# Escape characters that are special in a regex.
#
# @param str
#	The string to escape
# @return
#	The escaped string
#
function esc(str,
             a, cnt, i) {
	cnt = split(str, a, "")
	str = ""
	for (i = 1; i <= cnt; ++i) {
		str = str (a[i] ~ /[][$^.*+\\-]/ ? "\\" : "") a[i]
	}
	return str
}

##
# Compress the given path.
#
# Eliminate `./` and `../` from the given path.
#
# This can be broken by symlink shenanigans.
#
# @param path
#	The path to change
# @return
#	The compressed path
#
function compact(path) {
	# remove: ^./
	# remove: /./
	while(sub( /^\.\//, "", path));
	while(sub(/\/\.\//, "/", path));
	# remove: ^foo/../
	# remove: /foo/../
	#             | Single char
	#             |     | Two chars, first one not a dot
	#             |     |            | 2 chars, second one not a dot
	#             |     |            |            | 3+ chars
	while(sub( /^([^\/]|[^\/\.][^\/]|[^\/][^\/\.]|[^\/][^\/][^\/]+)\/\.\.\//, "", path));
	while(sub(/\/([^\/]|[^\/\.][^\/]+|[^\/][^\/\.]|[^\/][^\/][^\/]+)\/\.\.\//, "/", path));
	return path
}

##
# Generate context specific flags.
#
# @param file
#	The file to generate flags for
# @param fmt
#	A format string to embed each flag in, e.g. " ${COMPFLAGS%s}"
# @return
#	A hierarchy of flags for the given file
#
function flags(file, fmt,
               result) {
	result = ""
	while (file) {
		result = sprintf(fmt, "_" file) result
		sub(/\/?[^\/]*$/, "", file)
	}
	result = sprintf(fmt, "") result
	gsub(/[\/:.-]/, "_", result)
	return result
}

##
# Convert a file name to an object file name.
#
# The object suffix is taken from the `OBJSUFX` global.
#
# @param file
#	The file name to convert
# @return
#	The corresponding object file name
#
function src_to_obj(file) {
	sub(/\.[^\/\.]*$/, OBJSUFX, file)
	return file
}

##
# Convert a file name to an origin.
#
# This can be used in conjunction with ORIGIN to determine whether
# there is an object for an origin. E.g. `src/foo.c` and `src/foo.h`
# both have the origin `src/foo` and `ORIGIN["src/foo"]` points to
# `foo.o`.
#
# @param file
#	The file name to convert
# @return
#	The file name stripped of its suffix
#
function src_to_origin(file) {
	sub(/\.[^\/\.]*$/, "", file)
	return file
}

##
# Convert an object to a binary file name.
#
# The object and binary file name suffixes are taken from the `OBJSUFX`
# and `BINSUFX` globals.
#
# @param file
#	The file name to convert
# @return
#	The corresponding object file name
#
function obj_to_bin(obj) {
	sub(esc(OBJSUFX) "$", BINSUFX, obj)
	return obj
}

##
# Recursively get all objects required to link with the given object.
#
# Uses `ORIGIN` and `INCLUDES` to track dependencies.
#
# @param obj
#	The object to start from
# @return
#	A space separated list of object files
#
function linklist(obj,
                  aobjs, aincludes, file, result) {
	if (obj in aobjs) {
		return
	}
	aobjs[obj]
	split(INCLUDES[obj], aincludes, /[ \t\n]+/)
	for (key in aincludes) {
		file = aincludes[key]
		obj = ORIGIN[src_to_origin(file)]
		if (obj in INCLUDES) {
			linklist(obj, aobjs)
		}
	}
	for (obj in aobjs) {
		result = result " " obj
	}
	return result
}
