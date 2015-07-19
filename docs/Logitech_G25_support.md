Windows
-------

The G25 should be fully supported in Windows without any special steps.

Linux
-----

When initially plugged in, the G25 will be in compatibility mode, which restricts the steering range and disables the clutch pedal and some of the gears on the H-shifter. VDrift includes two tools that can be used to change G25 modes. Either tool can be used; use whichever tool works the best for you. If you want to use force feedback, you'll need to patch your kernel because when set to native mode, the G25 change its product id, and this product id is not known yet by the kernel ( have a look here [Enabling force feedback in kernel](Enabling_force_feedback_in_kernel.md))

### usbtool

The tool can be found in **tools/usbtool-0.1.tar.gz**. The tool requires libusb development headers to be installed (package libusb-dev on ubuntu intrepid, for example) as well as the swig package. Run `./build.sh` and then run `./usbtool` to see the options available. To set the wheel to native mode and the range to 900 degrees, run:

    sudo ./usbtool g25-set-range-wheel-900

and

    sudo ./usbtool g25-set-extended-mode

This will likely disconnect the joystick driver. To reconnect it run:

    sudo rmmod joydev;sudo rmmod usbhid;sudo modprobe usbhid

### G25manage

The tool can be found in **tools/G25manage**. The tool requires libusb development headers to be installed (package libusb-dev on ubuntu intrepid, for example). Run `make` to compile the tool, then run:

    sudo ./G25manage --nativemode

and

    sudo ./G25manage --range 900

The wheel should now support the entire turning radius as well as the clutch pedal.

### LTWheelConf

LTWheelConf is a new tool based on G25manage: <https://github.com/TripleSpeeder/LTWheelConf>

### Automatically enable native mode

If your distribution uses udev (such as Ubuntu), you can put this in **/etc/udev/rules.d/90-g25-wheel.rules** to automatically run G25manage when the wheel is plugged in:

    SUBSYSTEM!="usb", GOTO="g25_rules_end"
    ACTION!="add", GOTO="g25_rules_end"
    ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c294", RUN+="/usr/local/bin/G25manage --nativemode"
    #ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", RUN+="/usr/local/bin/G25manage --range 900"
    LABEL="g25_rules_end"
    # for a joystick detected by the kernel event interface, with a model name "G25_Racing_Wheel change the permissions on the device file
    # and add a symlink to the event device file
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", SYMLINK+="input/G25event"
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", MODE="0664", GROUP="games"
    # No deadzone for the wheel on the G25 in native mode
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", RUN+="/usr/local/bin/G25manage --evdev=/dev/input/G25event --deadzone=0 --axis=0"
    # No deadzone for the clutch pedal on the G25 in native mode
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", RUN+="/usr/local/bin/G25manage --evdev=/dev/input/G25event --deadzone=0 --axis=1"
    # No deadzone for the break pedal on the G25 in native mode
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", RUN+="/usr/local/bin/G25manage --evdev=/dev/input/G25event --deadzone=0 --axis=2"
    # No deadzone for the throttle pedal on the G25 in native mode
    KERNEL=="event[0-9]*", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="c299", RUN+="/usr/local/bin/G25manage --evdev=/dev/input/G25event --deadzone=0 --axis=5"

After creating that file and copying the G25manage binary to **/usr/local/bin**, run `/etc/init.d/udev reload` (or `service udev reload` on Ubuntu karmic) and you no longer have to manually run G25manage.

<Category:Configuration>
