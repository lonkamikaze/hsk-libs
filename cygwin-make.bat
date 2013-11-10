@echo off
sed -n "/^# | Target/,/^#$/s/# //p" Makefile
set /P TARGETS="Enter targets (leave empty for default): "
bash -lc "cd %CD% && make %TARGETS%"
pause