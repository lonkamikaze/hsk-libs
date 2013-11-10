@echo off
set /P TARGETS="Enter targets (leave empty for default): "
bash -lc "cd %CD% && make %TARGETS%"
pause