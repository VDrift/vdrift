#!/bin/bash

# This simple script installs all the VDrift data into the proper places. It is
# only used for the Linux binary package. If you are using the source distribution
# of VDrift please install with SCons.

if [ $# -ne "2" ]
then
	echo "This script installs the VDrift binary and data into the proper places."
	echo "Make sure you run it as a root-level user."
	echo
	echo "Usage:"
	echo "$0 /path/to/vdrift/binary /path/to/vdrift/data"
	echo
	echo "Example:"
	echo "$0 /usr/games/bin /usr/share/games/vdrift"
	echo
	echo "Here the vdrift exectuable will be /usr/games/bin/vdrift and the data will go"
	echo "in /usr/share/games/vdrift. Make sure that the binary directory is in your PATH."
	echo
	echo "For help or more information visit <http://vdrift.net/>"
	echo
	exit 1
fi

bin_dir=$1
data_dir=$2

echo
echo "Installing VDrift executable..."
if cp -fv vdrift $bin_dir
then
	echo "Done."
else
	echo "Failed. Are you sure you're running as root?"
	exit 1
fi

echo
echo "Installing VDrift data..."
if mkdir $data_dir && cp -rfv data/* $data_dir/
then
	echo "Done."
else
	echo "Failed. Are you sure you're running as root?"
	exit 1
fi

echo "Making sure default VDrift.config has the proper data directory..."
if $bin_dir/vdrift -defaultdatadir $data_dir
then
	echo "Done."
else
	echo "Failed. Something has broken. Please report this bug to the VDrift project."
	exit 1
fi

echo "Finished installing VDrift! You may now run VDrift by simply typing"
echo "vdrift"
echo 
echo "Enjoy!"
