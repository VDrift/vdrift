[SCons](http://scons.org/) is a replacement for autotools written in Python.

Compile
-------

To [compile VDrift](Compiling.md), simply run SCons. It reads the root level SCons configuration file, SConstruct, as well as the configuration files in subdirectories (called SConscript), when it is run to see what to do.

    scons

SCons starts by checking your system for available libraries. If things go well, this will look something like this:

    you@yourcomputer:~/games/vdrift$ scons
    scons: Reading SConscript files ...
    Checking for main() in C library GL... yes
    Checking for main() in C library GLU... yes
    Checking for main() in C library openal... yes
    Checking for C++ header file SDL/SDL.h... yes
    Checking for C++ header file SDL/SDL_image.h... yes
    Checking for C++ header file SDL/SDL_net.h... yes
    scons: done reading SConscript files.

Now, SCons will begin compiling VDrift, one file at a time, starting with the Vamos files and ending with linking the main executable. When changes to the source files are made, `scons` must be run again to update the build.

Clean
-----

When building a project it is sometimes necessary to "clean" a build (remove all files produced by the build). This can be done with the `-c` flag:

    scons -c

All the object and binary executable files will be removed. Then when you run `scons` again, all the files will be rebuilt.

Help
----

If you wish to list the options available at build time you may do so by executing `scons -h`. This will show all the available options, their default values, and their current values.

Quiet
-----

If you want SCons to give less verbose output when compiling use the `-Q` option.

<Category:Development>
