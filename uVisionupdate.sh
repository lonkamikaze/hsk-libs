#!/bin/sh -f

IFS='
'

overlays="$(awk -f scripts/overlays.awk $(find src/ -name \*.c))"

cp uVision/hsk_libs.uvproj uVision/hsk_libs.uvproj.bak
awk -f scripts/xml.awk uVision/hsk_libs.uvproj.bak \
	-search:OverlayString \
	-set:"$overlays" \
	-select:/ \
	-print > uVision/hsk_libs.uvproj \
		&& rm uVision/hsk_libs.uvproj.bak \
		|| mv uVision/hsk_libs.uvproj.bak uVision/hsk_libs.uvproj

