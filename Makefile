BUILDDIR=	bin.sdcc
CC=		sdcc
CFLAGS=		-mmcs51 --xram-loc 0xF000 --xram-size 3072 -I inc/

OBJSUFX=	.rel
HEXSUFX=	.hex

DATE:=		$(shell date +%Y-%m-%d)
DATE!=		date +%Y-%m-%d

USERSRC:=	$(shell find src/ -name \*.h -o -name \*.txt)
USERSRC!=	find src/ -name \*.h -o -name \*.txt

DEVSRC:=	$(shell find src/ -name \*.\[hc] -o -name \*.txt)
DEVSRC!=	find src/ -name \*.\[hc] -o -name \*.txt

build:

_BUILD_MK:=	$(shell sh scripts/build.sh src/ > build.mk)
_BUILD_MK!=	sh scripts/build.sh src/ > build.mk

# Gmake style, works with FreeBSD make, too
include build.mk

html: html/user html/dev html/contrib

html/contrib:
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
	@echo PROJECT_NUMBER=user-${DATE} > doc/.conf
	@cat doxygen.public.conf doc/.conf | doxygen -

doc-private: ${DEVSRC} doxygen.public.conf doxygen.private.conf
	@rm -rf doc-private || true
	@mkdir -p doc-private
	@echo PROJECT_NUMBER=dev-${DATE} > doc-private/.conf
	@cat doxygen.public.conf doxygen.private.conf doc-private/.conf \
		| doxygen -
	@cp -r contrib doc-private/html/

pdf: pdf/hsk_libs-user.pdf pdf/hsk_libs-dev.pdf

pdf/hsk_libs-user.pdf: doc/latex/refman.pdf
	@mkdir -p pdf
	@cp doc/latex/refman.pdf "pdf/hsk_libs-user.pdf"

pdf/hsk_libs-dev.pdf: doc-private/latex/refman.pdf
	@mkdir -p pdf
	@cp doc-private/latex/refman.pdf "pdf/hsk_libs-dev.pdf"

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

