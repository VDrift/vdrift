#!/bin/sh

# Script to compile VDrift for you

#cd "$VDRIFT_DIR"

echo "--VDrift Windows compilation script--"
echo
echo "Make sure you're running this from your VDrift folder, like so:"
echo "sh tools/win/bin/build_vdrift.sh"
echo

echo

scons release=1 || exit

cp build/vdrift.exe .
cp tools/win/dll/*.dll .

echo
echo "Make sure you check get the VDrift data!  See:"
echo "http://wiki.vdrift.net/Getting_the_development_version#Checking_out_the_data"

#echo "Running tests..."
#./vdrift -test || exit
