README:

VDrift is a car racing simulator for multiple platforms. It is an open source project licensed under GPL v3 that can be modified under the terms of this license.
Project's Homepage: http://vdrift.net/
Documentation: http://wiki.vdrift.net/Main_Page
Source repositories: https://github.com/VDrift
Data repository: http://sourceforge.net/projects/vdrift/


HARDWARE:

VDrift requires a relatively fast CPU. A 2 GHz or better clock speed is recommended, although it should be possible to run VDrift with a 1 GHz or better CPU.
On a typical run, VDrift might consume 300 MiB of memory space. 512 MiB of memory may be sufficient on some operating systems. 1 GiB or more is recommended, and required for larger tracks.
VDrift requires a recent nVidia or ATI graphics card. A nVidia GeForce 7-series or better (or the equivalent ATI card) is recommended in order to enable all the visual effects. By reducing or disabling some of the display options, it should be possible to play VDrift with a nVidia GeForce 2 or better. Users should install the newest version of the device drivers for their graphics card to run VDrift. Intel graphics cards are not supported.


DEPENDENCIES:

Make sure you have all the required libraries and build tools. Make sure you also have the development files for each of the libraries.

The required build tools include:
	g++ - The GNU C++ compiler
	scons - A replacement for Make

The required libraries include:
	libsdl - Simple Direct Media Layer
	libglew - OpenGL extension utilities
	sdl-gfx - Graphics drawing primitives library for SDL
	sdl-image - Image file loading library for SDL
	vorbisfile - File loading library for the ogg vorbis format
	libvorbis - The Vorbis General Audio Compression Codec
	bullet C++ Libraries (BulletCollision, BulletDynamics, LinearMath)
	libcurl - For managing data download from the net.
	libarchive - API for managing compressed files.

Your Linux distribution may have different package names and/or bundled differently. The list above should give enough information to search for applicable packages within your distribution's package manager.
For Ubuntu, all the required packages may be installed using this command:
sudo apt-get install g++ scons libsdl-gfx1.2-dev libsdl-image1.2-dev libsdl-net1.2-dev libvorbis-dev libglew-dev libcurl-dev libarchive-dev


COMPILING AND INSTALLING:

For compiling you'll need to download the source code package and your platform dependencies.
For downloading the source code from the repository execute "git clone https://github.com/VDrift/vdrift.git".
After this change your directory to the root of the sources "cd vdrift"

Donwload your platform dependencies:
- Windows: git clone https://github.com/VDrift/vdrift-win.git
- Mac OS: git clone https://github.com/VDrift/vdrift-mac.git

To compile vdrift type "scons" command in the root directory of the sources. You can use some flags to enable options, for example "scons arch=a64 release=1" will compile for a 64 bits machine and turn off debug.

For installing run "sudo scons install". You can also use diverse flags in this step like "scons install prefix=/usr/local" for setting the prefix to install vdrift.

Run "scons -h" for seing all the modificable flags.


EXPERIMENTAL:

The required build tools include:
	premake4 - This tool will generate project files which will suit your needs.
	
Once everything is downloaded use the command "premake4 --help" to see what project files do you want to generate (codeblocks, gmake, xcode3...).
Type "premake4 <action>" and your project files will appear in a folder under the name of "build".

Windows dependencies repo contains a premake4 executable. Use "vdrift-win\premake4 --help", "vdrift-win\premake4 <action>".
Use your favourite compiler or ide to modify or compile the game.
