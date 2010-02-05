@echo off
echo.
echo --VDrift Windows compilation script--
echo.
echo Make sure you're running this from your VDrift folder, like so:
echo tools\win\bin\build_vdrift.bat

echo.
set def=include\definitions.h
if exist %def% goto DONE
echo %def% not found. Generating %def%
echo #ifndef _DEFINITIONS_H>%def%
echo #define _DEFINITIONS_H>>%def%
echo #define SETTINGS_DIR ".vdrift">>%def%
echo #define DATA_DIR "./data">>%def%
echo #define VERSION "%date:~-4%-%date:~3,2%-%date:~0,2%">>%def%
echo #define REVISION "latest">>%def%
echo #endif>>%def%
:DONE

echo.
call scons release=1

echo.
echo Copying files:
xcopy /d build\vdrift.exe .
xcopy /d tools\win\dll\*.dll .

echo.
echo Make sure you check get the VDrift data!  See:
echo http://wiki.vdrift.net/Getting_the_development_version#Checking_out_the_data