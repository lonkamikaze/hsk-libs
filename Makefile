#
# Provides targets to build code with SDCC, generate documentation etc.
#
# | Target            | Function
# |-------------------|---------------------------------------------------
# | build (default)   | Builds a .hex file and dependencies
# | all               | Builds a .hex file and every .c library
# | dbc               | Builds C headers from Vector dbc files
# | printEnv          | Used by scripts to determine project settings
# | uVision           | Run uVisionupdate.sh
# | html              | Build html documentation
# | pdf               | Build pdf documentation
# | clean-build       | Remove build output
# | clean-doc         | Remove doxygen output for user doc
# | clean-doc-private | Remove doxygen output for dev doc
# | clean-doc-dbc     | Remove doxygen output for dbc doc
# | clean-stale       | Clean no longer required files, not managed by HG
# | clean             | Clean everything
# | zip               | Create a .zip archive in the parent directory
#
# Override the following settings in Makefile.local if needed.
#
# | Assignment        | Function
# |-------------------|---------------------------------------------------
# | AWK               | The awk interpreter
# | BUILDDIR          | SDCC output directory
# | CC                | Compiler
# | CFLAGS            | Compiler flags
# | CPP               | C preprocesser used by several scripts
# | CONFDIR           | Location for configuration files
# | CANPROJDIR        | Path to the CAN project
# | DBCDIR            | Location for generated DBC headers
# | GENDIR            | Location for generated code
# | INCDIR            | Include directory for contributed headers
# | OBJSUFX           | The file name suffix for object files
# | HEXSUFX           | The file name suffix for intel hex files
# | DATE              | System date, for use when hg is not available
# | VERSION           | Version of the project
# | USERSRC           | The list of source files to generate the user doc
# | DEVSRC            | The list of source files from this project
# | PROJECT           | The name of this project
#

# Build with SDCC.
BUILDDIR=	bin.sdcc
CC=		sdcc
CFLAGS=		-I${INCDIR} -I${GENDIR}

# Sane default for uVisionupdate.sh.
CPP=		cpp

# AWK interpreter.
AWK?=		awk

# Configuration files.
CONFDIR=	conf

# Generateded headers.
GENDIR=		gen
DBCDIR=		${GENDIR}/dbc

# Locate related projects.
CANPROJDIR=	../CAN

# Include directories from the related projects.
INCDIR=		inc

# File name suffixes for sdcc/XC800_Fload.
OBJSUFX=	.rel
HEXSUFX=	.hex

# The system date format.
DATE:=		$(shell date +%Y-%m-%d)
DATE!=		date +%Y-%m-%d

# Use hg version with date fallback.
VERSION:=	$(shell hg tip 2> /dev/null | ${AWK} '/^changeset/ {print $$2}' || echo ${DATE})
VERSION!=	hg tip 2> /dev/null | ${AWK} '/^changeset/ {print $$2}' || echo ${DATE}

# List of public source files, for generating the user documentation.
USERSRC:=	$(shell find src/ -name \*.h -o -name main.c -o -name \*.txt -o -name examples)
USERSRC!=	find src/ -name \*.h -o -name main.c -o -name \*.txt -o -name examples

# List of all source files for generating dependencies and documentation.
DEVSRC:=	$(shell find src/ -name \*.\[hc] -o -name \*.txt -o -name examples)
DEVSRC!=	find src/ -name \*.\[hc] -o -name \*.txt -o -name examples

# Name of this project.
PROJECT:=	$(shell pwd | xargs basename)
PROJECT!=	pwd | xargs basename

#
# No more overrides.
#

# Local config
_LOCAL_MK:=	$(shell test -f Makefile.local || touch Makefile.local)
_LOCAL_MK!=	test -f Makefile.local || touch Makefile.local ; echo

# Gmake style, works with FreeBSD make, too
include Makefile.local

build:

.PHONY: ${GENDIR}/sdcc.mk ${GENDIR}/dbc.mk ${GENDIR}/build.mk

# Create the generated content directory
${GENDIR}:
	@mkdir -p ${GENDIR}

# Configure SDCC
${GENDIR}/sdcc.mk: ${GENDIR}
	@env CC="${CC}" sh scripts/sdcc.sh ${CONFDIR}/sdcc > $@

# Generate dbc
${GENDIR}/dbc.mk: ${GENDIR}
	@sh scripts/dbc.sh ${CANPROJDIR}/ > $@

# Generate build
${GENDIR}/build.mk: dbc ${GENDIR}
	@env CPP="${CPP}" AWK="${AWK}" \
	     sh scripts/build.sh src/ ${INCDIR}/ ${GENDIR}/ > $@

.PHONY: build all dbc

# Generate headers from CANdbs
dbc: ${GENDIR}/dbc.mk
	@${MAKE} DBCDIR=${DBCDIR} -f ${GENDIR}/dbc.mk $@

${DBCDIR}: dbc

# Perform build stage
build all: ${GENDIR}/sdcc.mk ${GENDIR}/build.mk dbc
	@env BUILDDIR="${BUILDDIR}" \
	     OBJSUFX="${OBJSUFX}" HEXSUFX="${HEXSUFX}" \
	     CC="${CC}" CFLAGS="${CFLAGS}" \
	     ${MAKE} -r -f ${GENDIR}/sdcc.mk -f ${GENDIR}/build.mk $@

.PHONY: printEnv uVision µVision

printEnv:
	@echo export PROJECT=\"${PROJECT}\"
	@echo export CANPROJDIR=\"${CANPROJDIR}\"
	@echo export GENDIR=\"${GENDIR}\"
	@echo export INCDIR=\"${INCDIR}\"
	@echo export CPP=\"${CPP}\"
	@echo export AWK=\"${AWK}\"

uVision µVision:
	@sh uVisionupdate.sh

html: html/user html/dev html/dbc html/contrib

html/contrib: contrib
	@rm -rf html/contrib || true
	@mkdir -p html
	@cp -r contrib html/

html/user: doc
	@rm -rf html/user || true
	@mkdir -p html
	@cp -r doc/html html/user

html/dev: doc-private
	@rm -rf html/dev || true
	@mkdir -p html
	@cp -r doc-private/html html/dev

html/dbc: doc-dbc
	@rm -rf html/dbc || true
	@mkdir -p html
	@cp -r doc-dbc/html html/dbc

doc: ${USERSRC} ${CONFDIR}/doxygen.public
	@rm -rf doc || true
	@mkdir -p doc
	@echo PROJECT_NAME=\"${PROJECT}-user\" >> doc/.conf
	@echo PROJECT_NUMBER=${VERSION} >> doc/.conf
	@cat ${CONFDIR}/doxygen.public doc/.conf | doxygen -

doc-private: ${DEVSRC} ${CONFDIR}/doxygen.public ${CONFDIR}/doxygen.private
	@rm -rf doc-private || true
	@mkdir -p doc-private
	@echo PROJECT_NAME=\"${PROJECT}-dev\" >> doc-private/.conf
	@echo PROJECT_NUMBER=${VERSION} >> doc-private/.conf
	@cat ${CONFDIR}/doxygen.public ${CONFDIR}/doxygen.private \
	 doc-private/.conf | doxygen -

doc-dbc: ${DBCDIR} ${CONFDIR}/doxygen.dbc
	@rm -rf doc-dbc || true
	@mkdir -p doc-dbc
	@echo PROJECT_NAME=\"${PROJECT}-dbc\" >> doc-dbc/.conf
	@echo PROJECT_NUMBER=${VERSION} >> doc-dbc/.conf
	@echo INPUT=${DBCDIR} >> doc-dbc/.conf
	@echo STRIP_FROM_PATH=${GENDIR} >> doc-dbc/.conf
	@cat ${CONFDIR}/doxygen.dbc doc-dbc/.conf | doxygen -

pdf: pdf/${PROJECT}-user.pdf pdf/${PROJECT}-dev.pdf pdf/${PROJECT}-dbc.pdf

pdf/${PROJECT}-user.pdf: doc/latex/refman.pdf
	@mkdir -p pdf
	@cp doc/latex/refman.pdf "pdf/${PROJECT}-user.pdf"

pdf/${PROJECT}-dev.pdf: doc-private/latex/refman.pdf
	@mkdir -p pdf
	@cp doc-private/latex/refman.pdf "pdf/${PROJECT}-dev.pdf"

pdf/${PROJECT}-dbc.pdf: doc-dbc/latex/refman.pdf
	@mkdir -p pdf
	@cp doc-dbc/latex/refman.pdf "pdf/${PROJECT}-dbc.pdf"

doc/latex/refman.pdf: doc
	@cd doc/latex/ && ${MAKE}

doc-private/latex/refman.pdf: doc-private
	@cd doc-private/latex/ && ${MAKE}

doc-dbc/latex/refman.pdf: doc-dbc
	@cd doc-dbc/latex/ && ${MAKE}

.PHONY: clean clean-doc clean-doc-private clean-doc-dbc \
        clean-build clean-stale

clean: clean-doc clean-doc-private clean-doc-dbc clean-build clean-stale

clean-doc:
	@rm -rf doc || true

clean-doc-private:
	@rm -rf doc-private || true

clean-doc-dbc:
	@rm -rf doc-dbc || true

clean-build:
	@rm -rf ${BUILDDIR} ${GENDIR} || true

clean-stale:
	@rm -f build.mk sdcc.mk dbc.mk || true

.PHONY: zip

zip: pdf
	@hg status -A | ${AWK} '$$1 != "I" {sub(/. /, "${PROJECT}/"); print}' | (cd .. && zip ${PROJECT}-${VERSION}.zip -\@ -r ${PROJECT}/pdf)

