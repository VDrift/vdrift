Force feedback is currently only supported on Linux.

Prerequisites
-------------

-   a steering wheel supporting constant force effects (like a Logitech Momo Racing force feedback steering wheel)
-   Linux
-   HID\_FF is enabled in your kernel
-   your device's USB ID is in **kernel-source/drivers/usb/input/hid-ff.c**
-   the ff-utils test program ffcfstress works for you. Note the device name you use with this program (should be **/dev/input/eventX**).
-   write permission to **/dev/input/eventX**
-   VDrift SVN r1547 or later

If you need to recompile your kernel to enable force feedback, you can have a look here [Enabling force feedback in kernel](Enabling_force_feedback_in_kernel "wikilink")

Enabling force feedback in VDrift
---------------------------------

Once you've met the prerequisites, recompile vdrift like this:

`scons force_feedback=1`

Now open your [VDrift.config](VDrift_config "wikilink"). Find the section `[ joystick ]` and add the following line to the joystick section somewhere:

`ff_device = /dev/input/event0`

Change event0 to whatever device you should use (the one that worked with ffcfstress).

Start up VDrift. The console will print whether or not force feedback initialization succeeded. Start a practice game. You should feel a force effect on your steering wheel based on the aligning moment force from the front tires.

<Category:Configuration>
