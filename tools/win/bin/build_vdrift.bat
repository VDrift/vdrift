@echo off
echo.
echo --VDrift Windows compilation script--
echo.
echo Make sure you're running this from your VDrift folder, like so:
echo tools\win\bin\build_vdrift.bat

echo.
call scons release=1

echo.
echo Copying files:
xcopy /d build\vdrift.exe .
xcopy /d tools\win\dll\*.dll .

echo.
echo Make sure you check get the VDrift data!  See:
echo http://wiki.vdrift.net/Getting_the_development_version#Checking_out_the_data