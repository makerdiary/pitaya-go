@ECHO off
@TITLE	Root Certificate Downloader - Microchip Technology Inc
setlocal EnableDelayedExpansion

:: %1 com port eg 1 for COM1

if /I "a%1a" == "aa" (
  echo No comport specified
  goto usage
) else (
  if /I "a%1a" == "a0a" (
  	echo Comport should NOT be 0
  	goto usage
  ) else (
	set COMPORT=-port %1
  )
)

goto START

:USAGE
echo Usage %0 (comport)
pause
exit /b 2


:START

set /a c=0
set seq=
for %%X in (..\binary\*.cer) do (
	set /a c+=1
	@set seq=!seq! %%X
)

root_certificate_downloader -n %c% %seq% -no_wait %COMPORT% -e
