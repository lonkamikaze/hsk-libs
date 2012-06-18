#!/usr/bin/awk -f

function getIncludes() {
	if ($0 ~ /\/\*.*\*\//) {
		sub("/\*.*\*/", "");
	}
	
	if ($0 ~ /\/\*/ ) {
		sub("/\*.*", "");
		comment[FILENAME] = 1;
	}
	
	if ($0 ~ /\*\// ) {
		sub(".*\*/", "");
		comment[FILENAME] = 0;
	}
	
	if (!comment[FILENAME] && ($0 ~ /^#include[[:space:]]+"/)) { 
		sub("^#include[[:space:]]+\"", "");
		sub("\".*", "");
		include = FILENAME;
		sub("/[^/]*$", "/", include);
		include = include $0;
		while (include ~ "/\\./") {
			sub("/\\./", "/", include);
		}
		while (include ~ "/\\.\\./") {
			sub("[^/]*/\\.\\./", "", include);
		}
		includes[FILENAME] = includes[FILENAME] " " include;
		files[include];
		sub("\\.h", ".c", include);
		files[include] = 1;
	}

	if (!comment[FILENAME] && ($0 ~ /^#include[[:space:]]+</)) { 
		sub("^#include[[:space:]]+<", "");
		sub(">.*", "");
		while ($0 ~ "/\\./") {
			sub("/\\./", "/");
		}
		while ($0 ~ "/\\.\\./") {
			sub("[^/]*/\\.\\./", "");
		}
		for (root in roots) {
			include = root $0
			if (system("test -f " include) == 0) {
				includes[FILENAME] = includes[FILENAME] " " include;
				files[include];
				sub("\\.h", ".c", include);
				files[include] = 1;
			}
		}
	}
}

BEGIN {
	for (i = 1; i < ARGC; i++) {
		if (ARGV[i] ~ /\/$/) {
			roots[ARGV[i]] = 1;
		} else {
			files[ARGV[i]] = 1;
		}
	}

	do {
		more = 0;
		for (file in files) {
			if (getline <file > 0) {
				FILENAME = file;
				getIncludes();
				more = 1;
			}
		}
	} while (more);

	for (file in files) {
		if (includes[file]) {
			print file ":" includes[file] "\n";
		}
	}
}

