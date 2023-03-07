This page shows how to compile VDrift from source. It assumes you have downloaded the source code either by getting the source package from the [latest release](Downloading.md), or by [getting the development version](Getting_the_development_version.md).

Windows
-------

### Building with MSYS2

-   This is the recommended method to build VDrift. It does not require vdrift-win.
-   Install [MSYS2](http://sourceforge.net/p/msys2/wiki/MSYS2%20installation/) and update local packages.
-   Install build tools.

        pacman -S mingw-w64-x86_64-gcc scons

-   Install VDrift dependencies.

        pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-bullet mingw-w64-x86_64-curl mingw-w64-x86_64-libvorbis mingw-w64-x86_64-gettext mingw-w64-x86_64-zlib

-   Adjust your paths, if necessary:

        export PATH=$PATH:/mingw64/bin
        export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/mingw64/lib/pkgconfig

-   Build VDrift using MinGW-w64 Win64 Shell.

        scons

-   For more build options run

        scons --help

### Building with Code::Blocks/MinGW (obsolete)

-   Download the [latest MinGW](http://sourceforge.net/projects/mingw/files/). When asked to select components for install, you only need the base system and the g++ compiler.
-   Your PATH Environment Variable should contain the MinGW bin path (e.g. C:\\MinGW\\bin;).
-   Download [Code::Blocks nightly](http://forums.codeblocks.org/index.php/board,20.0.html)
-   Run the following command from the **vdrift** folder to generate the build files.

        vdrift-win\premake4 codeblocks

### Building with MSVC (obsolete)

-   Run the appropriate command from the **vdrift** folder to generate the project files.
    -   Microsoft Visual C++ 2008:

            vdrift-win\premake4 vs2008

    -   Microsoft Visual C++ 2010:

            vdrift-win\premake4 vs2010

OS X / macOS
----

There are two ways to compile on macOS:
1. Xcode
2. scons

In both cases, you must clone [vdrift-mac](https://github.com/VDrift/vdrift-mac.git) in `vdrift` directory:

        git clone https://github.com/VDrift/vdrift-mac.git

and checkout the svn `data` repositoy in `vdrift` folder (almost 3.4G):

        svn checkout https://svn.code.sf.net/p/vdrift/code/vdrift-data data

Then you can compile and run `vdrift` opening with `Xcode` the project: `vdrift-mac/vdrift.xcodeproj`
or compiling using `scons`.

### Using `Xcode`
The VDrift OS X project requires [Xcode](http://developer.apple.com/xcode/) 3.2 or later. The latest version is free on the [Mac App Store](http://itunes.apple.com/us/app/xcode/id422352214?mt=12&ls=1).

Open **vdrift/vdrift-mac/vdrift.xcodeproj** and in Xcode 4 or higher click Product -&gt; Build, otherwise hit Build -&gt; Build.
More details in [vdrift-mac](https://github.com/VDrift/vdrift-mac.git)

### Using `scons`
Compiling using `scons`, is a little bit tedious for macOS, but anyway, it can be done in similar way it's explained below for [Ubuntu](#ubuntu); install the dependencies using `brew`.
For example:

        brew install scons

After the first compilation, you must copy two libraries from `vdrift-mac/Libraries` to `build/vdrift-mac.app/Contents/Framewors`:

        cp vdrift-mac/Libraries/libiconv.2.dylib build/vdrift.app/Contents/Frameworks/
        cp vdrift-mac/Libraries/libintl.8.dylib build/vdrift.app//Contents/Frameworks/

and then `data` directory into `build` folder, you checked out before in `vdrift` folder.

Finally:
        open build/vdrift.app

Linux
-----

### Prerequisites

The required build tools include:

-   [g++](http://gcc.gnu.org/) - The GNU C++ compiler
-   [SCons](http://scons.org/scons) - A replacement for Make

The required libraries include:

-   [Bullet](http://bulletphysics.org/wordpress/) - Open Source Physics Library (minimum version 2.83).
-   [libcurl](http://curl.haxx.se/) - Multiprotocol file transfer library (minimum version 7.21.6)
-   [libvorbis](http://xiph.org/vorbis/) - The Vorbis General Audio Compression Codec Library (minimum version 1.2.0)
-   [SDL](http://www.libsdl.org/) - Simple DirectMedia Layer Library (minimum version 2.0.0)

Make sure you have all the required libraries and build tools. Make sure you also have the development files for each of the libraries. Your Linux distribution may have different package names and/or bundled differently. The list above should give enough information to search for applicable packages within your distribution's package manager.

#### Fedora

All required packages can be installed using this command:

    sudo yum install bullet-devel gcc-c++ gettext libvorbis-devel scons SDL2-devel curl-devel zlib-devel

#### Ubuntu

All required packages can be installed using this command:

    sudo apt-get install g++ scons gettext libsdl2-dev libbullet-dev libvorbis-dev libcurl4-gnutls-dev libz-dev

### Compiling

To compile VDrift just run SCons in the root directory of the sources.

    scons

#### Optional: Compile Options

You can use one or more compile options. To compile with optimization for a certain platform, you can use the arch flag:

    scons arch=a64

Compiling VDrift in release mode will turn off debugging options, and enable more compiler optimizations. VDrift runs much more quickly in release mode:

    scons release=1

You can get a list of all compile time options with:

    scons --help

These options are probably best left off the first time you compile. If you have problems compiling or running VDrift, it is easier to debug with them off. Once you verify that VDrift is compiling, then recompile with these optimizations to improve performance.

### Installing

VDrift does not need to be installed to work and you can run it from the folder where you compiled it. If you do want to install, use the SCons build target install.

    sudo scons install

#### Optional: Installation Location

There are also some install options - to change where VDrift is installed, use the prefix flag:

    scons install prefix=/usr/local

### Cleaning

Building the project creates several artefacts that do not need to be stored, because they can be regenerated on demand. Cleaning them up can be done with SCons, too:

    scons --clean

To remove all additional temporary files run:

    rm -rf .sconf_temp/ .sconsign.dblite config.log vdrift.conf

FreeBSD
-------

To compile VDrift on FreeBSD, use the ports system.

### Latest Release

If you downloaded a release, simply run `make` on the **vdrift** port:

    cd /usr/ports/games/vdrift && make install clean clean-depends

### Development Version

If you downloaded the development version, copy the **vdrift** and **vdrift-data** ports to **vdrift-devel** and **vdrift-data-devel**:

    cd /usr/ports/games && cp -rf vdrift vdrift-devel && cp -rf vdrift-data vdrift-data-devel

To compile, run `make` on the newly-created **vdrift-devel** port:

    cd /usr/ports/games/vdrift-devel && make install clean clean-depends

<Category:Development>
