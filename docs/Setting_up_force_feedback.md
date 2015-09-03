VDrift is using SDL2 haptics functionality to implement force feedback.

Start up VDrift. The console will print whether or not force feedback initialization succeeded. Start a practice game. You should feel a force effect on your steering wheel based on the aligning moment force from the front tires.

Force feedback requirements on Linux (obsolete)
-----------------------------------------------

-   a steering wheel supporting constant force effects (like a Logitech Momo Racing force feedback steering wheel)
-   HID\_FF is enabled in your kernel
-   your device's USB ID is in **kernel-source/drivers/usb/input/hid-ff.c**
-   the ff-utils test program ffcfstress works for you. Note the device name you use with this program (should be **/dev/input/eventX**).
-   write permission to **/dev/input/eventX**
-   VDrift SVN r1547 or later

If you need to recompile your kernel to enable force feedback, you can have a look here [Enabling force feedback in kernel](Enabling_force_feedback_in_kernel.md)

<Category:Configuration>
