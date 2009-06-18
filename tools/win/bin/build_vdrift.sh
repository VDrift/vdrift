#!/bin/sh

# Script to compile VDrift for you

#cd "$VDRIFT_DIR"

echo "--VDrift Windows compilation script--"
echo
echo "Make sure you're running this from your VDrift folder, like so:"
echo "sh tools/win/bin/build_vdrift.sh"
echo

if [ ! -d "bullet-2.73" ]
then
	echo "Bullet folder doesn't exist; untarring it"
	tar zxvf bullet-2.73-sp1.tgz || exit
else
	echo "Bullet folder already exists:  good!"
fi

echo

scons release=1 || exit

cp build/vdrift.exe .
cp tools/win/dll/*.dll .

echo
echo "Make sure you check get the VDrift data!  See:"
echo "http://wiki.vdrift.net/Getting_the_development_version#Checking_out_the_data"

#echo "Running tests..."
#./vdrift -test || exit