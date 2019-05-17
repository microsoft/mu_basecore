@setlocal
@set ToolName=%~n0%
@set PYTHONPATH=%PYTHONPATH%;%BASE_TOOLS_PATH%\Source\Python
@REM MU_CHANGE: add quotes around %PYTHON_COMMAND% to support spaces in path
@"%PYTHON_COMMAND%" -m %ToolName%.%ToolName% %*
