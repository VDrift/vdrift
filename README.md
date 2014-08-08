VDrift - A Car Racing Simulator for Multiple Platforms
======================================================

VDrift is a cross-platform, open source driving simulation made with drift
racing in mind. The driving physics engine was recently re-written from scratch
but was inspired and owes much to the Vamos physics engine. It is released under
the GNU General Public License (GPL) v3. It is currently available for FreeBSD,
Linux, Mac OS X and Windows.

Mission Statement
-----------------

The goals of the VDrift project are:

- to be a high-quality, open source racing simulation featuring enjoyable and
  challenging gameplay;
- to take advantage of modern computing hardware to accurately simulate vehicle
  physics in rich and immersive racing environments; and
- to provide a platform for creative experimentation to a community of
  developers and artists.

Saying More Than a Thousand Words
---------------------------------

![](vdrift/raw/2f19c79de4fac0c326fa099dba7d9f19362552d0/miura_vdrift_899.jpg)

See also
--------

- [VDrift's wiki on VDrift](http://wiki.VDrift.net/About_the_project)
- [VDrift's wiki on licensing](http://wiki.VDrift.net/License)

Hardware Requirements
=====================

CPU
---

VDrift requires a relatively fast CPU. A 2 GHz or better clock speed is
recommended, although it should be possible to run VDrift with a 1 GHz or better
CPU.

GPU
---

VDrift requires an OpenGL 2.0 capable graphics card, OpenGL 3.3 is recommended.

A nVidia GeForce 8 Series or ATI Radeon HD 4800 Series card is recommended in
order to enjoy all visual effects. By reducing or disabling some of the
graphics options, it should be possible to play VDrift with a nVidia GeForce 6 Series
or an ATI Radeon 9000 Series (AGP).

Hard Disk Drive
---------------

When following this readme, one will typically need about 5.5 GiB hard disk
space. VDrift source code and data sum up to about 3.5 GiB at the build location
and installation requires about 2 GiB of space including dependencies.

Network Transfer
----------------

About 2 - 3 GiB network transfer volume are required for downloading VDrift, its
data and dependencies.

RAM
---

VDrift consumes 300 MiB of memory space on a typical run, so 512 MiB of
memory is the minimum requirement. Though, 1 GiB or more memory is recommended,
especially for larger tracks.

See also
--------

- [VDrift's wiki on hardware requirements](http://wiki.VDrift.net/Hardware_requirements)

Downloading VDrift
==================

VDrift consists of two essential parts; the core source code and content data.

Downloading VDrift Source
-------------------------

Download the source code from the repository

    git clone https://github.com/VDrift/vdrift.git VDrift

Downloading VDrift Data
-----------------------

VDrift Data is expected to reside in a folder called *data* in the root of
VDrift, so change your directory to the root of the sources:

    cd VDrift

Currently the VDrift Data is still hosted at Sourceforge, so to getting it
requires checking out the repository with subversion:

    svn checkout svn://svn.code.sf.net/p/vdrift/code/vdrift-data data

See also
--------

- [VDrift's wiki on getting the lastest release](http://wiki.vdrift.net/Getting_the_latest_release)
- [VDrift's wiki on getting the development version](http://wiki.vdrift.net/Getting_the_development_version)

Installing Dependencies
=======================

VDrift requires several tools and libraries at build time. Make sure you have
installed all required libraries, each correspondent development files and all build
tools, as listed below.

Build Tools
-----------

The required build tools include:

- g++ - The GNU C++ compiler.
- scons - A replacement for Make.

    Needed on Linux, only.
- premake - Automatically builds configuration from source code.

    Needed Windows and MacOS, only.
- gettext - GNU libraries and utilities for producing multi-lingual messages

Libraries
---------

The required libraries include:

- bullet - Open Source Physics Library (minimum version 2.78).
- libcurl - Multiprotocol file transfer library (minimum version 7.21.6).
- libvorbis - The Vorbis General Audio Compression Codec Library (minimum version 1.2.0).
- vorbisfile - File loading library for the ogg vorbis format.
- sdl - Simple DirectMedia Layer Library (minimum version 2.0.0).
- sdl-image - Image file loading library (minimum version 2.0.0).

Installing Dependencies on FreeBSD
----------------------------------

Please seek advice from the wiki.

Installing Dependencies on Linux
--------------------------------

Your Linux distribution may have different package names and/or bundled
differently. The lists above should give enough information to search for
applicable packages within your distribution's package manager.

Below following are instructions for Ubuntu Linux. Please seek advice for other
distributions from the wiki.

Installing Dependencies on Fedora Linux
---------------------------------------

All required packages can be installed using this command:

    sudo yum install bullet-devel gcc-c++ libvorbis-devel scons SDL2-devel SDL2_image-devel curl-devel


Installing Dependencies on Ubuntu Linux
---------------------------------------

Ubuntu 12.04 does not include libbullet and SDL2 packages. They are available in following ppas though:

    sudo add-apt-repository ppa:roblib/ppa
    sudo add-apt-repository ppa:zoogie/sdl2-snapshots

All required packages can be installed using this command:

    sudo apt-get install g++ scons libsdl2-dev libsdl2-image-dev libbullet-dev libvorbis-dev libcurl4-gnutls-dev


Installing Dependencies on Mac OS
---------------------------------

Please seek advice from the wiki.

Installing Dependencies on Windows
----------------------------------

- Install the [official Git for Windows](http://code.google.com/p/msysgit/downloads/list)

    It will take a little while to compile Git.

- Install [msysGit](http://code.google.com/p/msysgit/downloads/list)

    Installing *git bash only* will be sufficient.

- Install [Subversion](http://www.sliksvn.com/en/download)

    Other packages are available, too but this one is tested.

    Install with the *typical* profile.

- Install [Code::Blocks](http://www.codeblocks.org/downloads/26)

    Download the one containing *mingw* in the filename.

    Install with the *MinGW Compiler Suite*.

After this, all following git commands are to be entered in a Git Bash, which
can be opened via the start menu.

See also
--------

- [VDrift's wiki on software requirements](http://wiki.VDrift.net/Software_requirements)

Injecting Build Dependencies
============================

Libraries that can not be installed need to present at the git root of VDrift.

Injecting Build Dependencies on FreeBSD
---------------------------------------

Please seek advice from the wiki.

Injecting Build Dependencies on Linux
-------------------------------------

On Linux all needed libraries can be installed using the package management,
usually. Therefore it is not necessary to inject them.

Injecting Build Dependencies on Mac OS
--------------------------------------

Download platform specific build dependencies on Mac OS:

    cd VDrift
    git clone https://github.com/VDrift/vdrift-mac.git

Injecting Build Dependencies on Windows
---------------------------------------

Download platform specific build dependencies on Windows:

    cd VDrift
    git clone https://github.com/VDrift/vdrift-win.git

See also
--------

- [VDrift's wiki on injecting build dependencies](http://wiki.vdrift.net/Injecting_Build_Dependencies)

Compiling VDrift
================

The downloaded source code needs to be compiled to be executable. The process to
do this depends on the platform on that the compiling is done.

Compiling VDrift on FreeBSD
---------------------------

Please seek advice from the wiki.

Compiling VDrift on Linux
-------------------------

To compile VDrift you only need to run `scons` in the root directory of the
sources. You can use some flags to enable options. To compile for a 64 bits
machine, turn off debugging, use the bullet physics engine you just installed
and install VDrift to the default directory run:

    scons arch=a64 release=1 extbullet=1 prefix=/usr/local

Compiling VDrift on Mac OS
--------------------------

Please seek advice from the wiki.

Compiling VDrift on Windows
---------------------------

Please seek advice from the wiki.

See also
--------

- [VDrift's wiki on compiling](http://wiki.VDrift.net/Compiling)

Installing VDrift
=================

In order to integrate with the platform the compiled executables need to be
installed in the system.

Installing VDrift on FreeBSD
----------------------------

Please seek advice from the wiki.

Installing VDrift on Linux
--------------------------

To install VDrift, you need to run `sudo scons install` in the root directory of
the sources. You can use some flags in this step, too. To set the prefix to the
default location explicitely for example run:

    sudo scons install prefix=/usr/local

Installing VDrift on Mac OS
---------------------------

Please seek advice from the wiki.

Installing VDrift on Windows
----------------------------

Please seek advice from the wiki.

See also
--------

- [VDrift's wiki on installing](http://wiki.VDrift.net/Installing)

Cleaning up VDrift
==================

Building the project creates several artifacts that do not need to be stored,
because they can be regenerated on demand.

Cleaning up VDrift on FreeBSD
-----------------------------

Please seek advice from the wiki.

Cleaning up VDrift on Linux
---------------------------

Cleaning them up can be done with scons, too:

    scons --clean

To remove all additional temporary files:

    rm -rf .sconf_temp/ .sconsign.dblite config.log vdrift.conf

Cleaning up VDrift on Mac OS
----------------------------

Please seek advice from the wiki.

Cleaning up VDrift on Windows
-----------------------------

Please seek advice from the wiki.

See also
--------

- [VDrift's wiki on cleaning up](http://wiki.VDrift.net/Cleaning_up)

Everything further
==================

For configuring, running and extending the game, for playing, contributing and
developing please search the wiki for an article on your topic:

- [VDrift's wiki front page](http://wiki.VDrift.net/Main_Page)
