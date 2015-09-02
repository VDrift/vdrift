This page documents how to package VDrift for releasing. It assumes you have [compiled](Compiling.md) VDrift already.

Common
------

The **data/settings/updates.config** file should be updated to contain the cars that will be included in the release along with their SVN revision numbers.

Windows
-------

### Preparation

-   You will need [Nullsoft installer system](http://nsis.sourceforge.net/).
-   You should also change the version on line 26 of **vdrift/vdrift-win/vdrift.nsi** to the date of the release.

### Packaging

Right click on the **vdrift/vdrift-win/vdrift.nsi** file and select "Compile script". The installer will be created.

OS X
----

### Preparation

-   In Xcode, open the "Info" tab and change "Bundle versions string, short" (CFBundleShortVersionString) and "Bundle version" (CFBundleVersion) to the date of the release.
-   If you have a code signing identity in Xcode, change the build setting Code Signing Identity to the appropriate profile - usually the automatic Mac Developer.

### Packaging

In Xcode 4 or higher:

-   Go to Product -&gt; Edit Scheme, change the build configuration of Run to Release and click OK.
-   Click Product -&gt; Build.

In other Xcode versions:

-   Switch to the "Release" build style using the popup-menu in the toolbar.
-   Hit Build -&gt; Build.

Run **vdrift/vdrift-mac/DMG.app**, entering the date of the release and selecting the VDrift.app you just built when asked. The disk image will be created.

### Developer ID

If you are a member of Apple's Mac Developer Program, you can use your Developer ID to sign the application and improve the experience on OS X Mountain Lion. Follow these instructions, instead of the ones above:

-   Click Product -&gt; Archive
-   When the organiser appears, select the newly built archive and click "Distribute". Select "Export Developer ID-signed Application" and follow the instructions.
-   Run **vdrift/vdrift-mac/DMG.app**, entering the date of the release and selecting the VDrift.app you just built when asked. The disk image will be created.

Linux
-----

### Binary packages with autopackage

#### Prerequisites

There are some things you need before you can build an autopackage.

First and most importantly, the autopackage development tools are needed. As of this writing, only the version in autopackage svn will correctly build the VDrift packages. The next release of the autopackage tools should have all the patches needed to build proper packages. BinReloc is used by VDrift but the code has been imported and you do not need to download this to build a package. You do however need apbuild.

Second, you must have two different compilers installed on your machine. This is to do the double-compiling so the vdrift binary will work on Linux no matter what version of glibc was used to build the distribution. Currently, the VDrift package setup uses g++-3.3 and g++-4.1, with gcc-4.1. Another known working setup is g++-3.3, g++-3.4, and gcc-3.4. The compiler versions are set in the **tools/autopackage/vdrift.apspec** file in [VDrift Git](Getting_the_development_version.md).

#### Building

The [autopackage](http://autopackage.org/) project is a very promising and useful next-generation packaging tool for Linux. It aims to help developers create packages that will work on any Linux distribution.

VDrift's autopackage creation is integrated into the SCons build system used to build the Linux version of VDrift. To build an autopackage, simply run

    scons autopackage

Two options are important when running this command, **minimal** and **release**. For instance,

    scons autopackage minimal=1

will build a package with the minimal data set. This can be combined with the release option to make a release-optimized package with the minimal data set:

    scons autopackage minimal=1 release=1

Turning these options to 0 turns them off. This affects the package built as one might expect. The package name reflects how it was built. For example:

    VDrift yyyy-mm-dd-dev-minimal.package
    VDrift yyyy-mm-dd-full.package

Usually the file is renamed to replace the space with a '-' character, so that the URL to the package does not have to contain a "%20".

### Source packages

Source packages are made in a similar way as autopackages using SCons.

    scons src-package

builds a package named similarly. This package is put into the build/ directory. Currently the build system does tar the source package after compiling it in the build/ directory, but it includes the build directory as a parent in the archive itself. Since this function is not used that much, it has been a low priority to fix, and it is also pretty simple to just run tar manually and create the package.

Before doing this however, it is useful to warn the user that there is no data in the source package and they must download one of the source packages in order to install the game. This can be done by replacing the file data/SConscript in the file with the file in the SVN repository data/SConscript.no\_data. This is a minor change made to the build system when source packages are made.

So, to make all this make sense, here are the commands to build a source package correctly:

    scons src-package
    cd build
    cp ../data/SConscript.no_data data/SConscript
    tar -jcvf vdrift-yyyy-mm-dd-src.tar.bz2 vdrift-yyyy-mm-dd-src/

Thus you end up with the file **vdrift-yyyy-mm-dd-src.tar.bz2** as a finished source package.

FreeBSD
-------

Todo

<Category:Development>
