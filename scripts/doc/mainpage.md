\mainpage HSK XC878 µC Library Build Scripts

This document contains the documentation for the `scripts` folder. Scripts
are written for AWK and Bourne Shell.

@see [PDF Version](hsk-libs-scripts.pdf)

\section compatibility Compatibility

The AWK scripts have been tested with the following interpreters:
- `awk version 20121220 (FreeBSD)`
  - Default AWK interpreter in BSD systems and OS-X
  - Also known as New AWK (NAWK)
  - This is a version of Brian Kernighan's AWK, one of the authors of
    <i>The AWK Programming Language</i>
- `mawk 1.3.3`
  - Default AWK interpreter in Ubuntu GNU/Linux and derivatives
  - This is Mike's AWK
- `GNU Awk 4.1.0, API: 1.0`
  - Default AWK interpreter in many GNU/Linux distributions

\section layout Layout

The `scripts` folder has the following layout:
- `scripts/`
  - Contains AWK and SH scripts for automatic build configuration,
    code generators and filters
- `scripts/doc/`
  - Contains the text for this document
- `scripts/templates.dbc2c/`
  - Contains the code templates for the dbc2c.awk script
