@ECHO OFF
REM Command file for Sphinx documentation

REM using delayed expansion of variables...
setlocal enableextensions enabledelayedexpansion

set SPHINXBUILD=sphinx-build
set ALLSPHINXOPTS=

if NOT "%PAPER%" == "" (
	set ALLSPHINXOPTS=-D latex_paper_size=%PAPER% %ALLSPHINXOPTS%
)

if "%1" == "" goto help

if "%1" == "help" (
	:help
	echo.Please use `make ^<target^>` where ^<target^> is one of
	echo.  html      to make standalone HTML files
	echo.  latex     to make LaTeX files, you can set PAPER=a4 or PAPER=letter
	goto end
)

if "%1" == "clean" (
	for /d %%i in (_build\*) do rmdir /q /s %%i
	del /q /s _build\*
	goto end
)

if "%1" == "html" (
  	%SPHINXBUILD% -b html %ALLSPHINXOPTS% . _build/html
	)
	echo.
	echo.Build finished. The HTML pages are in _build/html.
	goto end
)

:end
