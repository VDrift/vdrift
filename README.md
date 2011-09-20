VDrift - a car racing simulator for multiple platforms
======================================================

VDrift is a cross-platform, open source driving simulation made with drift
racing in mind. The driving physics engine was recently re-written from scratch
but was inspired and owes much to the Vamos physics engine. It is released under
the GNU General Public License (GPL) v3. It is currently available for Linux,
FreeBSD, Mac OS X and Windows.

Mission Statement
-----------------

The goals of the VDrift project are:

- to be a high-quality, open source racing simulation featuring enjoyable and
  challenging gameplay;
- to take advantage of modern computing hardware to accurately simulate vehicle
  physics in rich and immersive racing environments; and
- to provide a platform for creative experimentation to a community of
  developers and artists.

See also
--------

- [wiki.vdrift.net/About_the_project](http://wiki.vdrift.net/About_the_project)
- [wiki.vdrift.net/License](http://wiki.vdrift.net/License)

Hardware Requirements
=====================

CPU
---

VDrift requires a relatively fast CPU. A 2 GHz or better clock speed is
recommended, although it should be possible to run VDrift with a 1 GHz or better
CPU.

RAM
---

VDrift consumes 300 MiB of memory space on a typical run. 512 MiB of
memory may be sufficient on some operating systems. 1 GiB or more is
recommended, and required for larger tracks.

GPU
---

VDrift requires a recent nVidia or ATI graphics card. A nVidia GeForce 7-series
or better (or the equivalent ATI card) is recommended in order to enable all the
visual effects. By reducing or disabling some of the display options, it should
be possible to play VDrift with a nVidia GeForce 2 or better. Users should
install the newest version of the device drivers for their graphics card to run
VDrift. Intel graphics cards are not supported.

See also
--------

- [wiki.vdrift.net/Hardware_requirements](http://wiki.vdrift.net/Hardware_requirements)


DEPENDENCIES
============

Make sure you have all the required libraries and build tools. Make sure you
also have the development files for each of the libraries.

Build Tools
-----------

The required build tools include:

- g++ - The GNU C++ compiler
- scons - A replacement for Make

Libraries
---------

The required libraries include:

- libsdl - Simple Direct Media Layer
- libglew - OpenGL extension utilities
- sdl-gfx - Graphics drawing primitives library for SDL
- sdl-image - Image file loading library for SDL
- vorbisfile - File loading library for the ogg vorbis format
- libvorbis - The Vorbis General Audio Compression Codec
- bullet C++ Libraries (BulletCollision, BulletDynamics, LinearMath)
- libcurl - For managing data download from the net.
- libarchive - API for managing compressed files.

Your Linux distribution may have different package names and/or bundled
differently. The list above should give enough information to search for
applicable packages within your distribution's package manager.

For Ubuntu, all the required packages may be installed using this command:

    sudo apt-get install g++ scons libsdl-gfx1.2-dev libsdl-image1.2-dev libsdl-net1.2-dev libvorbis-dev libglew-dev libcurl4-nss-dev libarchive-dev
    
See also
--------

- [wiki.vdrift.net/Software_requirements](http://wiki.vdrift.net/Software_requirements)

Downloading
===========

For compiling you'll need to download the source code and your platform
dependencies.

Download VDrift
---------------

For downloading the source code from the repository execute

    git clone https://github.com/VDrift/vdrift.git

After this change your directory to the root of the sources

    cd vdrift

Download platform dependencies
------------------------------

Download the dependencies for your plattform:

- Windows: `git clone https://github.com/VDrift/vdrift-win.git`
- Mac OS: `git clone https://github.com/VDrift/vdrift-mac.git`

See also
--------

- [wiki.vdrift.net/Getting_the_latest_release](http://wiki.vdrift.net/Getting_the_latest_release)

Compiling
=========

To compile vdrift type `scons` command in the root directory of the sources. You
can use some flags to enable options, for example `scons arch=a64 release=1`
will compile for a 64 bits machine and turn off debug.

See also
--------

- [wiki.vdrift.net/Compiling](http://wiki.vdrift.net/Compiling)

Installing
==========

For installing run `sudo scons install`. You can also use diverse flags in this
step like `scons install prefix=/usr/local` for setting the prefix to install
vdrift.

Run `scons -h` for seeing all the modificable flags.

See also
--------

- [wiki.vdrift.net/Installing](http://wiki.vdrift.net/Installing)

Everything further
==================

For configuring, running and extending the game, for playing, contributing and developing please search the
wiki for an article on your topic:

- [wiki.vdrift.net/Main_Page](http://wiki.vdrift.net/Main_Page)