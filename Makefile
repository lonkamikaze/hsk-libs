BUILDDIR=	bin.sdcc
CC=		sdcc
CFLAGS=		-mmcs51 --xram-loc 0xF000 --xram-size 3072 -Iinc/ -I${CANDIR}

CANDIR=		../CAN/src

OBJSUFX=	.rel
HEXSUFX=	.hex

DATE:=		$(shell date +%Y-%m-%d)
DATE!=		date +%Y-%m-%d

VERSION:=	$(shell hg tip 2> /dev/null | awk '/^changeset/ {print $$2}' || echo ${DATE})
VERSION!=	hg tip 2> /dev/null | awk '/^changeset/ {print $$2}' || echo ${DATE}

USERSRC:=	$(shell find src/ -name \*.h -o -name \*.txt)
USERSRC!=	find src/ -name \*.h -o -name \*.txt -o -name examples

DEVSRC:=	$(shell find src/ -name \*.\[hc] -o -name \*.txt)
DEVSRC!=	find src/ -name \*.\[hc] -o -name \*.txt -o -name examples

PROJECT:=	$(shell pwd | xargs basename)
PROJECT!=	pwd | xargs basename

_LOCAL_MK:=	$(shell test -f Makefile.local || touch Makefile.local)
_LOCAL_MK!=	test -f Makefile.local || touch Makefile.local

include Makefile.local

build:

_BUILD_MK:=	$(shell sh scripts/build.sh src/ ${CANDIR}/ > build.mk)
_BUILD_MK!=	sh scripts/build.sh src/ ${CANDIR}/ > build.mk

# Gmake style, works with FreeBSD make, too
include build.mk

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

doc: ${USERSRC} doxygen.public.conf
	@rm -rf doc || true
	@mkdir -p doc
	@echo PROJECT_NAME=\"${PROJECT}-user\" >> doc/.conf
	@echo PROJECT_NUMBER=${VERSION} >> doc/.conf
	@cat doxygen.public.conf doc/.conf | doxygen -

doc-private: ${DEVSRC} doxygen.public.conf doxygen.private.conf
	@rm -rf doc-private || true
	@mkdir -p doc-private
	@echo PROJECT_NAME=\"${PROJECT}-dev\" >> doc-private/.conf
	@echo PROJECT_NUMBER=${VERSION} >> doc-private/.conf
	@cat doxygen.public.conf doxygen.private.conf doc-private/.conf \
		| doxygen -

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
	@rm -rf ${BUILDDIR} || true

zip: pdf
	@hg status -A | awk '$$1 != "I" {sub(/. /, "${PROJECT}/"); print}' | (cd .. && zip ${PROJECT}.${DATE}.zip -\@ -r ${PROJECT}/pdf)

