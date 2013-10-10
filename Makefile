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
# | clean             | Clean everything
# | zip               | Create a .zip archive in the parent directory
#
# Override the following settings in Makefile.local if needed.
#
# | Assignment        | Function
# |-------------------|---------------------------------------------------
# | BUILDDIR          | SDCC output directory
# | CC                | Compiler
# | CFLAGS            | Compiler flags
# | CPP               | C preprocesser used by several scripts
# | CONFDIR           | Location for configuration files
# | CANPROJDIR        | Path to the CAN project
# | DBCDIR            | Location for generated DBC headers
# | GENDIR            | Location for generated headers
# | INCDIR            | Include directory for contributed headers
# | CANDIR            | Include directory for CAN DB headers
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

# Configuration files.
CONFDIR=	conf

# Generateded headers.
GENDIR=		gen
DBCDIR=		${GENDIR}/dbc

# Locate related projects.
CANPROJDIR=	../CAN

# Include directories from the related projects.
INCDIR=		inc
CANDIR=		${CANPROJDIR}

# File name suffixes for sdcc/XC800_Fload.
OBJSUFX=	.rel
HEXSUFX=	.hex

# The system date format.
DATE:=		$(shell date +%Y-%m-%d)
DATE!=		date +%Y-%m-%d

# Use hg version with date fallback.
VERSION:=	$(shell hg tip 2> /dev/null | awk '/^changeset/ {print $$2}' || echo ${DATE})
VERSION!=	hg tip 2> /dev/null | awk '/^changeset/ {print $$2}' || echo ${DATE}

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
_LOCAL_MK!=	test -f Makefile.local || touch Makefile.local

# Gmake style, works with FreeBSD make, too
include Makefile.local

build:

# Configure SDCC.
_SDCC_MK:=	$(shell env CC="${CC}" sh scripts/sdcc.sh ${CONFDIR}/sdcc > sdcc.mk)
_SDCC_MK!=	env CC="${CC}" sh scripts/sdcc.sh ${CONFDIR}/sdcc > sdcc.mk

# Gmake style, works with FreeBSD make, too
include sdcc.mk

# Generate dbc
_DBC_MK:=	$(shell sh scripts/dbc.sh ${CANDIR}/ > dbc.mk)
_DBC_MK!=	sh scripts/dbc.sh ${CANDIR}/ > dbc.mk

# Gmake style, works with FreeBSD make, too
include dbc.mk

# Make sure DBCs are generated before the build scripts are created
_DBC_MK:=	$(shell ${MAKE} DBCDIR=${DBCDIR} -f dbc.mk dbc 1>&2)
_DBC_MK!=	${MAKE} DBCDIR=${DBCDIR} -f dbc.mk dbc 1>&2

# Generate build
_BUILD_MK:=	$(shell sh scripts/build.sh src/ ${INCDIR}/ ${GENDIR}/ > build.mk)
_BUILD_MK!=	sh scripts/build.sh src/ ${INCDIR}/ ${GENDIR}/ > build.mk

# Gmake style, works with FreeBSD make, too
include build.mk

printEnv::
	@echo export PROJECT=\"${PROJECT}\"
	@echo export CANPROJDIR=\"${CANPROJDIR}\"
	@echo export CANDIR=\"${CANDIR}\"
	@echo export INCDIR=\"${INCDIR}\"
	@echo export CPP=\"${CPP}\"

uVision µVision::
	@sh uVisionupdate.sh

html: html/user html/dev html/contrib

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

pdf: pdf/${PROJECT}-user.pdf pdf/${PROJECT}-dev.pdf

pdf/${PROJECT}-user.pdf: doc/latex/refman.pdf
	@mkdir -p pdf
	@cp doc/latex/refman.pdf "pdf/${PROJECT}-user.pdf"

pdf/${PROJECT}-dev.pdf: doc-private/latex/refman.pdf
	@mkdir -p pdf
	@cp doc-private/latex/refman.pdf "pdf/${PROJECT}-dev.pdf"

doc/latex/refman.pdf: doc
	@cd doc/latex/ && ${MAKE}

doc-private/latex/refman.pdf: doc-private
	@cd doc-private/latex/ && ${MAKE}

clean: clean-doc clean-doc-private clean-build

clean-doc:
	@rm -rf doc || true

clean-doc-private:
	@rm -rf doc-private || true

clean-build:
	@rm -rf ${BUILDDIR} ${GENDIR} || true

zip: pdf
	@hg status -A | awk '$$1 != "I" {sub(/. /, "${PROJECT}/"); print}' | (cd .. && zip ${PROJECT}-${VERSION}.zip -\@ -r ${PROJECT}/pdf)

