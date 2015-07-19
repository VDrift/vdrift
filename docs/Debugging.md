Debugging VDrift can help provide the developers with more information on problems, so they can be more effectively fixed. This article hopes to explain how to do this.

General
-------

VDrift outputs most of its debugging info on to the console via "cout" statements. There are some logs kept in ~/.vdrift/logs/ but they are not used much. To get more output on the console, users can simply use the **-verbose** option when running the game:

    vdrift -verbose

This will drastically increase the amount of debugging output. This may reduce game performance but makes it easier to get an idea of where problems are occurring.

Linux
-----

If the game crashes it is easiest to get information about the crash from GDB, the GNU Debugger.

### GDB

On Linux debugging can be done using **gdb**. First, VDrift must be compiled with debugging symbols. To turn on debugging symbols simply use the **release=0** option when running SCons:

    scons release=0

Now the binary (which is in the build/ directory if it is not installed) can be analyzed with GDB. Make sure you have the program **gdb** installed on your system, then run GDB on the vdrift binary (change "build/vdrift" to just "vdrift" if you ran **scons install**):

    gdb build/vdrift

This will put you on the gdb shell where you can then run the game. Here you can also specify any command line arguments to pass to the game. **-verbose** is added here as an example, but no options are necessary for debugging.

    (gdb) run -verbose

Now the game will run. You may notice that when running in GDB game performance is less than normal. This is a natural side effect of GDB and nothing to worry about.

#### Obtaining a backtrace

If the game crashes (commonly resulting in a "Segmentation Fault" error) while running in GDB it is possible to obtain a *backtrace* of the crash. This is basically the stack of function calls that happened to trigger the crash. To obtain a backtrace within GDB, wait for the game to crash, and then when returned to the GDB prompt, enter the **backtrace** command:

    (gdb) backtrace

The output of this command can be posted on the VDrift forums for the developers to see. Please follow up on your post, as we may have other questions for you or may need you to perform other tests to properly identify the problem.

<Category:Development>
