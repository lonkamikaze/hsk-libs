#!/bin/sh -f
#
# Updates the Keil µVision configuration, with the correct include paths
# and overlays.
#
# The list of overlays is generated using the overlays.awk script and
# the configuration is updated using the xml.awk script.
#

# Force noglob mode
set -f

IFS='
'

# Must not be set or GNU make produces problematic output on stdout
unset MAKELEVEL
eval "$(make printEnv)"

echo "Generating C-headers from DBCs ..." 1>&2
make dbc

echo "Preparing header include directories ..." 1>&2
_GENDIR="$(echo "$GENDIR" | tr '/' '\\')"

echo "Getting call tree changes for overlay optimisation ..." 1>&2
overlays="$($AWK -f scripts/overlays.awk $(find src/ -name *.c) -I$INCDIR -I$GENDIR)"
echo "$overlays" | sed -e 's/^/	/' -e 's/[[:cntrl:]]$//' 1>&2

echo "Updating uVision/hsk-libs.uvproj ..." 1>&2
cp uVision/hsk-libs.uvproj uVision/hsk-libs.uvproj.bak
$AWK -f scripts/xml.awk uVision/hsk-libs.uvproj.bak \
        -search:Target51/C51/VariousControls/IncludePath \
        -set:"..\\$_GENDIR" \
        -select:/ \
	-search:OverlayString \
	-set:"$overlays" \
	-select:/ \
	-print > uVision/hsk-libs.uvproj \
		&& rm uVision/hsk-libs.uvproj.bak \
		|| mv uVision/hsk-libs.uvproj.bak uVision/hsk-libs.uvproj

