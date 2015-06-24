VDrift - A Car Racing Simulator for Multiple Platforms
======================================================

VDrift is a cross-platform, open source driving simulation made with drift
racing in mind. The driving physics engine was recently re-written from scratch
but was inspired and owes much to the Vamos physics engine. It is released under
the GNU General Public License (GPL) v3. It is currently available for FreeBSD,
Linux, Mac OS X and Windows.

[![Build Status](https://travis-ci.org/VDrift/vdrift.svg?branch=master)](https://travis-ci.org/VDrift/vdrift) [![Coverity Status](https://scan.coverity.com/projects/3734/badge.svg)](https://scan.coverity.com/projects/3734)

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

Getting Started
---------------

- [Official website] (http://vdrift.net)
- [Wiki](http://wiki.vdrift.net/index.php?title=Main_Page)
- [Bug tracker](https://github.com/VDrift/vdrift/issues)
- [Hardware Requirements](http://wiki.vdrift.net/index.php?title=Requirements)

Downloading
===========

VDrift consists of two essential parts; source code and data.

Download the source code from the repository

    git clone https://github.com/VDrift/vdrift.git VDrift

VDrift data is expected to reside in a folder called *data* in the root of
VDrift, so change your directory to the root of the sources:

    cd VDrift

VDrift Data is still hosted at Sourceforge, so to getting it
requires checking out the repository with subversion:

    svn checkout svn://svn.code.sf.net/p/vdrift/code/vdrift-data data

See also
--------

- [Getting the development version](http://wiki.vdrift.net/index.php?title=Getting_the_development_version)

Compiling
=========

VDrift requires several tools and libraries at build time. Make sure you have
installed all required libraries, each correspondent development files and all build
tools, as listed below.

Build Tools
-----------

- g++ 4.8 or clang 3.4 - C++11 compiler: 
- [SCons](http://www.scons.org) - Build system.
- gettext - GNU libraries and utilities for producing multi-lingual messages

Libraries
---------

- [bullet](http://bulletphysics.org/wordpress) - Open Source Physics Library (minimum version 2.83).
- [libcurl](http://curl.haxx.se) - Multiprotocol file transfer library (minimum version 7.21.6).
- [libvorbis](http://xiph.org/vorbis) - The Vorbis General Audio Compression Codec Library (minimum version 1.2.0).
- [sdl](http://www.libsdl.org) - Simple DirectMedia Layer Library (minimum version 2.0.0).
- [sdl-image](https://www.libsdl.org/projects/SDL_image) - Image file loading library (minimum version 2.0.0).

Platform specific compiling instructions are available at
--------

- [Compiling](http://wiki.vdrift.net/index.php?title=Compiling)

Everything further
==================

For configuring, running and extending the game, for playing, contributing and
developing please search the wiki for an article on your topic:

- [VDrift wiki](http://wiki.vdrift.net/index.php?title=Main_Page)
