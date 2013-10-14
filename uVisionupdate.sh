#!/bin/sh -f

IFS='
'

eval "$(make printEnv)"

echo "Preparing header include directories ..." 1>&2
_GENDIR="$(echo "$GENDIR" | tr '/' '\\')"

echo "Getting call tree changes for overlay optimisation ..." 1>&2
overlays="$($AWK -f scripts/overlays.awk $(find src/ -name \*.c) -I$INCDIR -I$GENDIR)"
echo "$overlays" | sed -e 's/^/	/' -e 's/[[:cntrl:]]$//' 1>&2

echo "Updating uVision/hsk_libs.uvproj ..." 1>&2
cp uVision/hsk_libs.uvproj uVision/hsk_libs.uvproj.bak
$AWK -f scripts/xml.awk uVision/hsk_libs.uvproj.bak \
        -search:Target51/C51/VariousControls/IncludePath \
        -set:"..\\$_GENDIR" \
        -select:/ \
	-search:OverlayString \
	-set:"$overlays" \
	-select:/ \
	-print > uVision/hsk_libs.uvproj \
		&& rm uVision/hsk_libs.uvproj.bak \
		|| mv uVision/hsk_libs.uvproj.bak uVision/hsk_libs.uvproj

