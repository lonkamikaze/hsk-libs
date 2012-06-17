#!/bin/sh -f

IFS='
'

CANDIR=$(make -VCANDIR 2> /dev/null)
if [ -z "$CANDIR" ]; then
	eval "$(sed -Ene '/CANDIR=/s/CANDIR=[[:space:]]*(.*)/CANDIR="\1"/p' \
		Makefile Makefile.local)"
fi

overlays="$(awk -f scripts/overlays.awk $(find src/ -name \*.c))"

cp uVision/hsk_libs.uvproj uVision/hsk_libs.uvproj.bak
awk -f scripts/xml.awk uVision/hsk_libs.uvproj.bak \
        -search:IncludePath \
        -set:"../$CANDIR" \
        -select:/ \
	-search:OverlayString \
	-set:"$overlays" \
	-select:/ \
	-print > uVision/hsk_libs.uvproj \
		&& rm uVision/hsk_libs.uvproj.bak \
		|| mv uVision/hsk_libs.uvproj.bak uVision/hsk_libs.uvproj

