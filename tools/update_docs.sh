#!/bin/bash

# Batch script to update Doxygen documentation on vdrift.net

# name of archive to create
archive_filename=vdrift-doxygen-`date "+%Y%m%d"`.tar.bz2
# host where vdrift.net lives
remote_server=bujumbura.dreamhost.com
# directory to untar into
remote_public_dir=/home/thelusiv/vdrift.net/public_html

# Make sure we're at the latest version
svn up
# Delete previous version docs
rm -rf doxygen/trunk

# Export an environment variable used in Doxyfile
export TODAY=`date "+%Y/%m/%d"`
# Generate the documenation from the Doxyfile
doxygen > doxygen_`date "+%Y%m%d"`.log 
# Change name of dir
mv doxygen/html doxygen/trunk
# Create tarball archive for transfer
tar jcf $archive_filename doxygen/

# Transfer tarball to server
scp $archive_filename $remote_server:$remote_public_dir
# Login to remote server; remove old, untar new, remove archive
ssh $remote_server "\
rm -rf $remote_public_dir/doxygen/; \
tar jxf $remote_public_dir/$archive_filename -C $remote_public_dir; \
rm -f $remote_public_dir/$archive_filename"

# Remove local copy of archive
rm -f $archive_filename

