#!/bin/sh -f

IFS='
'

eval "$(make printEnv)"

echo "Preparing header include directories ..." 1>&2
CANDIR="$(echo "$CANDIR" | tr '/' '\\')"

echo "Getting call tree changes for overlay optimisation ..." 1>&2
overlays="$(awk -f scripts/overlays.awk $(find src/ -name \*.c))"
echo "$overlays" | sed -e 's/^/	/' -e 's/[[:cntrl:]]$//' 1>&2

echo "Updating uVision/hsk_dev.uvproj ..." 1>&2
cp uVision/hsk_libs.uvproj uVision/hsk_libs.uvproj.bak
awk -f scripts/xml.awk uVision/hsk_libs.uvproj.bak \
        -search:Target51/C51/VariousControls/IncludePath \
        -set:"..\\$CANDIR" \
        -select:/ \
	-search:OverlayString \
	-set:"$overlays" \
	-select:/ \
	-print > uVision/hsk_libs.uvproj \
		&& rm uVision/hsk_libs.uvproj.bak \
		|| mv uVision/hsk_libs.uvproj.bak uVision/hsk_libs.uvproj

