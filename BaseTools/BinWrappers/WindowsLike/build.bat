@setlocal
@set ToolName=%~n0%
REM MU_CHANGE: add quotes around %PYTHON_COMMAND% to support spaces in path
@"%PYTHON_COMMAND%" %BASE_TOOLS_PATH%\Source\Python\%ToolName%\%ToolName%.py %*
