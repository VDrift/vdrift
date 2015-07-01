Here is how to enable force feedback on Ubuntu 8.10 64 bits kernels (should also work on 32 bits ones):

For the G25 owners
------------------

If you use a G25 wheel, you need to patch your kernel to be able to use it in its native mode First create a patch file named G25.patch for the kernel ( patches for 2.6.27 and 2.6.28 are provided ).

### kernel 2.6.27

`diff -Naur linux-source-2.6.27/drivers/hid/usbhid/hid-ff.c linux-source-2.6.27.orig/drivers/hid/usbhid/hid-ff.c`
`--- linux-source-2.6.27/drivers/hid/usbhid/hid-ff.c    2008-10-10 00:13:53.000000000 +0200                     `
`+++ linux-source-2.6.27.orig/drivers/hid/usbhid/hid-ff.c       2009-02-23 22:21:27.000000000 +0100             `
`@@ -57,6 +57,7 @@                                                                                              `
`       { 0x46d, 0xc286, hid_lgff_init }, /* Logitech Force 3D Pro Joystick */                                  `
`       { 0x46d, 0xc294, hid_lgff_init }, /* Logitech Formula Force EX */                                       `
`       { 0x46d, 0xc295, hid_lgff_init }, /* Logitech MOMO force wheel */`
`+      { 0x46d, 0xc299, hid_lgff_init }, /* Logitech G25 wheel */`
`       { 0x46d, 0xca03, hid_lgff_init }, /* Logitech MOMO force wheel */`
` #endif`
` #ifdef CONFIG_LOGIRUMBLEPAD2_FF`
`diff -Naur linux-source-2.6.27/drivers/hid/usbhid/hid-lgff.c linux-source-2.6.27.orig/drivers/hid/usbhid/hid-lgff.c`
`--- linux-source-2.6.27/drivers/hid/usbhid/hid-lgff.c  2008-10-10 00:13:53.000000000 +0200`
`+++ linux-source-2.6.27.orig/drivers/hid/usbhid/hid-lgff.c     2009-02-23 22:23:22.000000000 +0100`
`@@ -55,6 +55,7 @@`
`       { 0x046d, 0xc286, ff_joystick },`
`       { 0x046d, 0xc294, ff_joystick },`
`       { 0x046d, 0xc295, ff_joystick },`
`+      { 0x046d, 0xc299, ff_joystick },`
`       { 0x046d, 0xca03, ff_joystick },`
` };`
`diff -Naur linux-source-2.6.27/drivers/hid/usbhid/hid-quirks.c linux-source-2.6.27.orig/drivers/hid/usbhid/hid-quirks.c`
`--- linux-source-2.6.27/drivers/hid/usbhid/hid-quirks.c        2009-03-13 18:54:14.000000000 +0100`
`+++ linux-source-2.6.27.orig/drivers/hid/usbhid/hid-quirks.c   2009-02-23 22:25:45.000000000 +0100`
`@@ -316,6 +316,7 @@`
` #define USB_DEVICE_ID_LOGITECH_HARMONY_64 0xc14f`
` #define USB_DEVICE_ID_LOGITECH_EXTREME_3D     0xc215`
` #define USB_DEVICE_ID_LOGITECH_WHEEL  0xc294`
`+#define USB_DEVICE_ID_LOGITECH_WHEELG25       0xc299`
` #define USB_DEVICE_ID_LOGITECH_ELITE_KBD      0xc30a`
` #define USB_DEVICE_ID_LOGITECH_KBD    0xc311`
` #define USB_DEVICE_ID_S510_RECEIVER   0xc50c`
`@@ -625,6 +626,7 @@`
`       { USB_VENDOR_ID_ELO, USB_DEVICE_ID_ELO_TS2700, HID_QUIRK_NOGET },`
`       { USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_EXTREME_3D, HID_QUIRK_NOGET },`
`       { USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_WHEEL, HID_QUIRK_NOGET },`
`+      { USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_WHEELG25, HID_QUIRK_NOGET },`
`       { USB_VENDOR_ID_MICROSOFT, USB_DEVICE_ID_WIRELESS_OPTICAL_DESKTOP_3_0, HID_QUIRK_NOGET },`
`       { USB_VENDOR_ID_PETALYNX, USB_DEVICE_ID_PETALYNX_MAXTER_REMOTE, HID_QUIRK_NOGET },`
`       { USB_VENDOR_ID_SUN, USB_DEVICE_ID_RARITAN_KVM_DONGLE, HID_QUIRK_NOGET },`

### kernel 2.6.28

`diff -Naur linux-source-2.6.28/drivers/hid/hid-core.c linux-source-2.6.28.patched/drivers/hid/hid-core.c`
`--- linux-source-2.6.28/drivers/hid/hid-core.c  2009-04-08 06:38:33.000000000 +0200`
`+++ linux-source-2.6.28.patched/drivers/hid/hid-core.c  2009-04-10 14:15:27.000000000 +0200`
`@@ -1290,6 +1290,7 @@`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_FORCE3D_PRO) },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_MOMO_WHEEL) },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_MOMO_WHEEL2) },`
`+       { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_G25_WHEEL) },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_RUMBLEPAD2) },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_MICROSOFT, USB_DEVICE_ID_SIDEWINDER_GV) },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_MICROSOFT, USB_DEVICE_ID_MS_NE4K) },`
`diff -Naur linux-source-2.6.28/drivers/hid/hid-ids.h linux-source-2.6.28.patched/drivers/hid/hid-ids.h`
`--- linux-source-2.6.28/drivers/hid/hid-ids.h   2008-12-25 00:26:37.000000000 +0100`
`+++ linux-source-2.6.28.patched/drivers/hid/hid-ids.h   2009-04-10 14:12:44.000000000 +0200`
`@@ -291,6 +291,7 @@`
` #define USB_DEVICE_ID_LOGITECH_FORCE3D_PRO     0xc286`
` #define USB_DEVICE_ID_LOGITECH_WHEEL   0xc294`
` #define USB_DEVICE_ID_LOGITECH_MOMO_WHEEL      0xc295`
`+#define USB_DEVICE_ID_LOGITECH_G25_WHEEL       0xc299`
` #define USB_DEVICE_ID_LOGITECH_ELITE_KBD       0xc30a`
` #define USB_DEVICE_ID_LOGITECH_KBD     0xc311`
` #define USB_DEVICE_ID_S510_RECEIVER    0xc50c`
`diff -Naur linux-source-2.6.28/drivers/hid/hid-lg.c linux-source-2.6.28.patched/drivers/hid/hid-lg.c`
`--- linux-source-2.6.28/drivers/hid/hid-lg.c    2008-12-25 00:26:37.000000000 +0100`
`+++ linux-source-2.6.28.patched/drivers/hid/hid-lg.c    2009-04-10 14:14:17.000000000 +0200`
`@@ -304,6 +304,8 @@`
`                .driver_data = LG_FF },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_MOMO_WHEEL2),`
`                .driver_data = LG_FF },`
`+       { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_G25_WHEEL),`
`+               .driver_data = LG_FF },`
`        { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, USB_DEVICE_ID_LOGITECH_RUMBLEPAD2),`
`                .driver_data = LG_FF2 },`
`        { }`

Enabling force feedback in the kernel
-------------------------------------

### Prepare to compile a kernel

Make sure you have everything needed to compile your kernel :

`sudo apt-get install fakeroot build-essential makedumpfile libncurses5 libncurses5-dev kernel-package`
`sudo apt-get build-dep linux`

Get the kernel source code :

`sudo apt-get install linux-source`

Now we are ready to start : Prepare your environment:

`mkdir ~/src`
`cd ~/src`
`tar xjvf /usr/src/linux-source-`<version>`.tar.bz2`
`cd linux-source-`<version>

### Apply the G25 patch if needed

Copy the G25.patch file into the source directory ( ~/src ) and

`patch -p1 <../G25.patch`

The output should be something like that:

`patching file drivers/hid/usbhid/hid-ff.c`
`patching file drivers/hid/usbhid/hid-lgff.c`
`patching file drivers/hid/usbhid/hid-quirks.c`

### Configure the new kernel

Get the running kernel configuration:

`` cp -vi /boot/config-`uname -r` .config ``

Adjust the kernel configuration:

`make oldconfig`
`make menuconfig`

Go to Device Drivers , and HID Devices and enable the following options

`Force feedback support (EXPERIMENTAL)`
`  PID device support`
`  Logitech devices support`
`  Logitech Rumblepad 2 support`
`  PantherLord/GreenAsia based device support`
`  ThrustMaster devices support`
`  Zeroplus based game controller support`

With the 2.6.27 kernel on Ubuntu 8.10, I have to disable paravirtualized guest support otherwise the kernel cleaning fail. This is not needed on 2.6.28 on Ubuntu 9.04 beta (2.6.28.11) Go to Processor type and features and disable the following:

`Paravirtualized guest support`

### Compile and package the new kernel

`make-kpkg clean`
`fakeroot make-kpkg --initrd --append-to-version=-ff kernel-image kernel-headers`

You now have 2 new package files in ~/src.

### Install the new kernel

`sudo dpkg -i linux-image-2.6.27.18-ff_2.6.27.18-ff-10.00.Custom_amd64.deb linux-headers-2.6.27.18-ff_2.6.27.18-ff-10.00.Custom_amd64.deb`

<Category:Configuration>
