@ECHO OFF
REM (C) 2018-2021 see Authors.txt
REM 
REM This file was part of MPC-BE and is now named TBD12 Video Renderer.
REM 
REM MPC-BE is free software; you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation; either version 3 of the License, or
REM (at your option) any later version.
REM
REM TBD12 Video Renderer is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with this program.  If not, see <http://www.gnu.org/licenses/>.

SETLOCAL
CD /D %~dp0

SET "TITLE=TBD12 Video Renderer"
SET "PROJECT=MpcVideoRenderer"

SET "MSBUILD_SWITCHES=/nologo /consoleloggerparameters:Verbosity=minimal /maxcpucount /nodeReuse:true"
SET "BUILDTYPE=Build"
SET "BUILDCFG=Release"
SET "SUFFIX="
SET "SIGN=False"

FOR %%A IN (%*) DO (
  IF /I "%%A" == "Debug" (
    SET "BUILDCFG=Debug"
    SET "SUFFIX=_Debug"
  )
  IF /I "%%A" == "Sign" (
    SET "SIGN=True"
  )
)

IF /I "%SIGN%" == "True" (
  IF NOT EXIST "%~dp0signinfo.txt" (
    CALL :SubMsg "WARNING" "signinfo.txt not found."
    SET "SIGN=False"
  )
)

CALL :SubVSPath
SET "TOOLSET=%VS_PATH%\Common7\Tools\vsdevcmd"

SET "LOG_DIR=_bin\logs"
IF NOT EXIST "%LOG_DIR%" MD "%LOG_DIR%"

CALL "%TOOLSET%" -no_logo -arch=x86
REM again set the source directory (fix possible bug in VS2017)
CD /D %~dp0
CALL :SubCompiling x86

CALL "%TOOLSET%" -no_logo -arch=amd64
REM again set the source directory (fix possible bug in VS2017)
CD /D %~dp0
CALL :SubCompiling x64

IF /I "%SIGN%" == "True" (
  SET FILES="%~dp0_bin\Filter_x86%SUFFIX%\%PROJECT%.ax" "%~dp0_bin\Filter_x64%SUFFIX%\%PROJECT%64.ax"
  CALL "%~dp0\sign.cmd" %%FILES%%
  IF %ERRORLEVEL% NEQ 0 (
    CALL :SubMsg "ERROR" "Problem signing files."
  ) ELSE (
    CALL :SubMsg "INFO" "Files signed successfully."
  )
)

FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define VER_MAJOR" "Include\Version.h"') DO (SET "VERMAJOR=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define VER_MINOR" "Include\Version.h"') DO (SET "VERMINOR=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define VER_BUILD" "Include\Version.h"') DO (SET "VERBUILD=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define VER_RELEASE" "Include\Version.h"') DO (SET "VERRELEASE=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_DATE" "revision.h"') DO (SET "REVDATE=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_HASH" "revision.h"') DO (SET "REVHASH=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_NUM" "revision.h"') DO (SET "REVNUM=%%A")
FOR /F "tokens=3,4 delims= " %%A IN (
  'FINDSTR /I /L /C:"define REV_BRANCH" "revision.h"') DO (SET "REVBRANCH=%%A")

IF /I "%VERRELEASE%" == "1" (
  SET "PCKG_NAME=%PROJECT%-%VERMAJOR%.%VERMINOR%.%VERBUILD%.%REVNUM%%SUFFIX%"
) ELSE (
  IF /I "%REVBRANCH%" == "master" (
    SET "PCKG_NAME=%PROJECT%-%VERMAJOR%.%VERMINOR%.%VERBUILD%.%REVNUM%_git%REVDATE%-%REVHASH%%SUFFIX%"
  ) ELSE (
    SET "PCKG_NAME=%PROJECT%-%VERMAJOR%.%VERMINOR%.%VERBUILD%.%REVNUM%.%REVBRANCH%_git%REVDATE%-%REVHASH%%SUFFIX%"
  )
)

CALL :SubDetectSevenzipPath
IF DEFINED SEVENZIP (
    IF EXIST "_bin\%PCKG_NAME%.zip" DEL "_bin\%PCKG_NAME%.zip"

    TITLE Creating archive %PCKG_NAME%.zip...
    START "7z" /B /WAIT "%SEVENZIP%" a -tzip -mx9 "_bin\%PCKG_NAME%.zip" ^
.\_bin\Filter_x86%SUFFIX%\%PROJECT%.ax ^
.\_bin\Filter_x64%SUFFIX%\%PROJECT%64.ax ^
.\distrib\Install_MPCVR_32.cmd ^
.\distrib\Install_MPCVR_64.cmd ^
.\distrib\Uninstall_MPCVR_32.cmd ^
.\distrib\Uninstall_MPCVR_64.cmd ^
.\distrib\Reset_Settings.cmd ^
.\Readme.md ^
.\history.txt ^
.\LICENSE
    IF %ERRORLEVEL% NEQ 0 CALL :SubMsg "ERROR" "Unable to create %PCKG_NAME%.zip!"
    CALL :SubMsg "INFO" "%PCKG_NAME%.zip successfully created"
)

TITLE Compiling %TITLE% [FINISHED]
TIMEOUT /T 3
ENDLOCAL
EXIT

:SubVSPath
FOR /f "delims=" %%A IN ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath -latest -requires Microsoft.Component.MSBuild') DO SET VS_PATH=%%A
EXIT /B

:SubDetectSevenzipPath
FOR %%G IN (7z.exe) DO (SET "SEVENZIP_PATH=%%~$PATH:G")
IF EXIST "%SEVENZIP_PATH%" (SET "SEVENZIP=%SEVENZIP_PATH%" & EXIT /B)

FOR %%G IN (7za.exe) DO (SET "SEVENZIP_PATH=%%~$PATH:G")
IF EXIST "%SEVENZIP_PATH%" (SET "SEVENZIP=%SEVENZIP_PATH%" & EXIT /B)

FOR /F "tokens=2*" %%A IN (
  'REG QUERY "HKLM\SOFTWARE\7-Zip" /v "Path" 2^>NUL ^| FIND "REG_SZ" ^|^|
   REG QUERY "HKLM\SOFTWARE\Wow6432Node\7-Zip" /v "Path" 2^>NUL ^| FIND "REG_SZ"') DO SET "SEVENZIP=%%B\7z.exe"
EXIT /B

:SubCompiling
TITLE Compiling %TITLE% - %BUILDCFG%^|%1...
MSBuild.exe %PROJECT%.sln %MSBUILD_SWITCHES%^
 /target:%BUILDTYPE% /p:Configuration="%BUILDCFG%" /p:Platform=%1^
 /flp1:LogFile=%LOG_DIR%\errors_%BUILDCFG%_%1.log;errorsonly;Verbosity=diagnostic^
 /flp2:LogFile=%LOG_DIR%\warnings_%BUILDCFG%_%1.log;warningsonly;Verbosity=diagnostic
IF %ERRORLEVEL% NEQ 0 (
  CALL :SubMsg "ERROR" "%PROJECT%.sln %BUILDCFG% %1 - Compilation failed!"
) ELSE (
  CALL :SubMsg "INFO" "%PROJECT%.sln %BUILDCFG% %1 compiled successfully"
)
EXIT /B

:SubMsg
ECHO. & ECHO ------------------------------
IF /I "%~1" == "ERROR" (
  CALL :SubColorText "0C" "[%~1]" & ECHO  %~2
) ELSE IF /I "%~1" == "INFO" (
  CALL :SubColorText "0A" "[%~1]" & ECHO  %~2
) ELSE IF /I "%~1" == "WARNING" (
  CALL :SubColorText "0E" "[%~1]" & ECHO  %~2
)
ECHO ------------------------------ & ECHO.
IF /I "%~1" == "ERROR" (
  ECHO Press any key to close this window...
  PAUSE >NUL
  ENDLOCAL
  EXIT
) ELSE (
  EXIT /B
)

:SubColorText
FOR /F "tokens=1,2 delims=#" %%A IN (
  '"PROMPT #$H#$E# & ECHO ON & FOR %%B IN (1) DO REM"') DO (
  SET "DEL=%%A")
<NUL SET /p ".=%DEL%" > "%~2"
FINDSTR /v /a:%1 /R ".18" "%~2" NUL
DEL "%~2" > NUL 2>&1
EXIT /B