#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <usb.h>
#include <time.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <stdint.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <asm/types.h>
#include <fcntl.h>

#include <linux/input.h>

/* this macro is used to tell if "bit" is set in "array"
 * it selects a byte from the array, and does a boolean AND 
 * operation with a byte that only has the relevant bit set. 
 * eg. to check for the 12th bit, we do (array[1] & 1<<4)
 */
#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))


clock_t start, end;
double cpu_time_used;

#define INTERFACE 0
#define ENDPOINT 1
#define VENDOR 0x046d
#define G25NORMAL 0xc294
#define G25EXTENDED 0xc299


/* Flag set by `--verbose'. */
static int verbose_flag;
struct usb_dev_handle {
  int fd;

  struct usb_bus *bus;
  struct usb_device *device;

  int config;
  int interface;
  int altsetting;

  /* Added by RMT so implementations can store other per-open-device data */
  void *impl_info;
};

struct usb_bus *busses;
struct usb_bus *bus;
struct usb_device *dev;
usb_dev_handle *usb_handle;

#define USB_ERROR_STR(format, args...) \
          if (verbose_flag) \
            fprintf(stderr, format, ## args);

struct usb_device *  usb_find_device(uint16_t vendor,uint16_t product){
  usb_init();
  usb_find_busses();
  usb_find_devices();

  busses = usb_get_busses();
  for (bus = busses; bus; bus = bus->next) {

    for (dev = bus->devices; dev; dev = dev->next) {
      if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product) {
        return dev;
      }
    }
  }
  return NULL;
}

int usb_rebind_kernel_driver_np()
{
  int ret;

  sleep(1);
  dev=usb_find_device(VENDOR,G25EXTENDED);
  if ( dev == NULL ) {
    dev=usb_find_device(VENDOR,G25NORMAL);
    if ( dev == NULL ) {
      fprintf(stderr,"usb_rebind_kernel_driver_np: no device found to rebind to kernel\n");
      return -ENODEV;
    }
  }


  usb_handle = usb_open(dev);
  if (!usb_handle) {
    fprintf(stderr,"usb_open: %s\n",strerror(errno));
  }
  if (verbose_flag) printf("Device opened\n");

  struct usbdevfs_ioctl command;
  command.ifno = 0;
  command.ioctl_code = USBDEVFS_CONNECT;
  command.data = NULL;
  ret = ioctl(usb_handle->fd, USBDEVFS_IOCTL, &command);
  if (ret == -1 && errno != EBUSY ){
    fprintf(stderr,"usb_rebind_kernel_driver_np: %i %s\n",errno,strerror(errno));
  }

  ret=usb_close(usb_handle);
  if (ret<0) {
    USB_ERROR_STR("usb_close: %s\n",strerror(-ret));
  }


  return ret;
}


int G25send_command(char command[7]) {

  int stat;

  usb_handle = usb_open(dev);
  if (!usb_handle) {
    fprintf(stderr,"usb_open: %s\n",strerror(errno));
  }
  if (verbose_flag) printf("Device opened\n");

  stat = usb_detach_kernel_driver_np(usb_handle, 0);
  if (stat<0 && verbose_flag) {
    fprintf(stderr,"usb_detach_kernel_driver_np: %i %s\n",stat,strerror(-stat));
  }
  if (verbose_flag) printf ("Kernel driver released\n");

  stat = usb_claim_interface(usb_handle, 0);
  if (stat<0) {
    if ( (stat == -EBUSY) && verbose_flag) fprintf(stderr, "usb_claim_interface: Check that you have permissions to write to %s/%s and, if you don't, that you set up udev correctly.\n", usb_handle->bus->dirname, usb_handle->device->filename);
    if ( (stat == -ENOMEM) && verbose_flag) fprintf(stderr, "usb_claim_interface: Insuffisant memory\n");
  }
  if (verbose_flag) printf ("USB interface claimed\n");

  stat=usb_interrupt_write(usb_handle, 1, command, sizeof(command), 100);
  if ( (stat < 0) ) { 
    if ( (stat == -ENODEV) && verbose_flag) fprintf(stderr, "usb_interrupt_write: No such device, changed identity ?\n");
  }
  else {
    if (verbose_flag) printf ("USB command sent\n");
    stat=usb_release_interface(usb_handle, 0);
    if (stat<0 && verbose_flag) {
      fprintf(stderr,"usb_release_interface: %i %s\n",stat,strerror(-stat));
    }
    if (verbose_flag) printf ("USB interface released\n");
  }

  stat=usb_close(usb_handle);
  if (stat<0) {
    USB_ERROR_STR("usb_close: %s\n",strerror(-stat));
  }

  stat=usb_rebind_kernel_driver_np();
  if (stat<0 && verbose_flag) {
    fprintf(stderr,"usb_rebind_kernel_driver_np: %i %s\n",stat,strerror(-stat));
  }
  if (verbose_flag) printf ("Kernel driver rebinded\n");

  return 0;
}

int G25native() {
  dev=usb_find_device(VENDOR,G25NORMAL);
  if ( dev != NULL ) {
    char setextended[] = { 0xf8, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00 };
    G25send_command(setextended);
  }
  else {
    printf ("Unable to find G25 device.\n");
  }
  return 0;
} 

int G25range(unsigned short int range) {
  dev=usb_find_device(VENDOR,G25EXTENDED);
  if ( dev != NULL ) {
    char setrange[] = { 0xf8, 0x81, range & 0x00ff , (range & 0xff00)>>8, 0x00, 0x00, 0x00 }; 
    G25send_command(setrange);
  }
  return 0;
} 

int G25reset() {
  int stat;
  dev=usb_find_device(VENDOR,G25EXTENDED);
  if ( dev == NULL ) {
    dev=usb_find_device(VENDOR,G25NORMAL);
    if ( dev == NULL ) {
      fprintf(stderr,"usb_rebind_kernel_driver_np: no device found to rebind to kernel\n");
      return -ENODEV;
    }
  }
  usb_handle = usb_open(dev);
  if (!usb_handle) {
    fprintf(stderr,"usb_open: %s\n",strerror(errno));
  }
  if (verbose_flag) printf("Device opened\n");

  stat=usb_reset(usb_handle);
  if (stat<0) {
    if ( (stat == -ENODEV) ) {
      if ( verbose_flag) fprintf(stderr, "usb_interrupt_write: No such device, changed identity ?\n");
    }
    else {
      fprintf(stderr,"usb_reset: %i %s\n",stat,strerror(-stat));
    }
  }
  stat=usb_close(usb_handle);
  if (stat<0) {
    USB_ERROR_STR("usb_close: %s\n",strerror(-stat));
  }
  if (verbose_flag) printf("Device resetted\n");
  return 0;
} 

int showcalibration(evdev)
     char *evdev;
{
  int fd = -1;
  uint8_t abs_bitmask[ABS_MAX/8 + 1];
  int yalv;
  float pourcent_deadzone;
  struct input_absinfo abs_features;

  if ((fd = open(evdev, O_RDONLY)) < 0) {
    perror("evdev open");
    return(1);
  }

  memset(abs_bitmask, 0, sizeof(abs_bitmask));
  if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
      perror("evdev ioctl");
  }

  printf("Supported Absolute axes:\n");

  for (yalv = 0; yalv < ABS_MAX; yalv++) {
      if (test_bit(yalv, abs_bitmask)) {
	  /* this means that the bit is set in the axes list */
	  printf("  Absolute axis 0x%02x (%d)", yalv, yalv);
	  switch ( yalv)
	      {
	      case ABS_X :
		  printf(" (X Axis) ");
		  break;
	      case ABS_Y :
		  printf(" (Y Axis) ");
		  break;
	      case ABS_Z :
		  printf(" (Z Axis) ");
		  break;
	      case ABS_RX :
		  printf(" (X Rate Axis) ");
		  break;
	      case ABS_RY :
		  printf(" (Y Rate Axis) ");
		  break;
	      case ABS_RZ :
		  printf(" (Z Rate Axis) ");
		  break;
	      case ABS_THROTTLE :
		  printf(" (Throttle) ");
		  break;
	      case ABS_RUDDER :
		  printf(" (Rudder) ");
		  break;
	      case ABS_WHEEL :
		  printf(" (Wheel) ");
		  break;
	      case ABS_GAS :
		  printf(" (Accelerator) ");
		  break;
	      case ABS_BRAKE :
		  printf(" (Brake) ");
		  break;
	      case ABS_HAT0X :
		  printf(" (Hat zero, x axis) ");
		  break;
	      case ABS_HAT0Y :
		  printf(" (Hat zero, y axis) ");
		  break;
	      case ABS_HAT1X :
		  printf(" (Hat one, x axis) ");
		  break;
	      case ABS_HAT1Y :
		  printf(" (Hat one, y axis) ");
		  break;
	      case ABS_HAT2X :
		  printf(" (Hat two, x axis) ");
		  break;
	      case ABS_HAT2Y :
		  printf(" (Hat two, y axis) ");
		  break;
	      case ABS_HAT3X :
		  printf(" (Hat three, x axis) ");
		  break;
	      case ABS_HAT3Y :
		  printf(" (Hat three, y axis) ");
		  break;
	      case ABS_PRESSURE :
		  printf(" (Pressure) ");
		  break;
	      case ABS_DISTANCE :
		  printf(" (Distance) ");
		  break;
	      case ABS_TILT_X :
		  printf(" (Tilt, X axis) ");
		  break;
	      case ABS_TILT_Y :
		  printf(" (Tilt, Y axis) ");
		  break;
	      case ABS_MISC :
		  printf(" (Miscellaneous) ");
		  break;
	      default:
		  printf(" (Unknown absolute feature) ");
	      }
	  if(ioctl(fd, EVIOCGABS(yalv), &abs_features)) {
	      perror("evdev EVIOCGABS ioctl");
	  }
	  pourcent_deadzone=(float)abs_features.flat*100/(float)abs_features.maximum;
	  printf("(min: %d, max: %d, flatness: %d (=%.2f%%), fuzz: %d)\n",
		 abs_features.minimum,
		 abs_features.maximum,
		 abs_features.flat,
                 pourcent_deadzone,
		 abs_features.fuzz);
          abs_features.flat=0;
/*	  if(ioctl(fd, EVIOCSABS(yalv), &abs_features)) {
	      perror("evdev EVIOCSABS ioctl");
	  } */
      }
  }

  close(fd);
  return(0);
}


int setdeadzonevalue(evdev,axisindex,deadzonevalue)
     char *evdev;
     int axisindex;
     __s32 deadzonevalue;
{
  int fd = -1;
  uint8_t abs_bitmask[ABS_MAX/8 + 1];
  float pourcent_deadzone;
  struct input_absinfo abs_features;

  if ((fd = open(evdev, O_RDONLY)) < 0) {
    perror("evdev open");
    return(1);
  }

  memset(abs_bitmask, 0, sizeof(abs_bitmask));
  if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
      perror("evdev ioctl");
  }

  if (axisindex < ABS_MAX) {
      if (test_bit(axisindex, abs_bitmask)) {
	  /* this means that the bit is set in the axes list */
	  printf("  Absolute axis 0x%02x (%d)", axisindex, axisindex);
	  switch ( axisindex)
	      {
	      case ABS_X :
		  printf(" (X Axis) ");
		  break;
	      case ABS_Y :
		  printf(" (Y Axis) ");
		  break;
	      case ABS_Z :
		  printf(" (Z Axis) ");
		  break;
	      case ABS_RX :
		  printf(" (X Rate Axis) ");
		  break;
	      case ABS_RY :
		  printf(" (Y Rate Axis) ");
		  break;
	      case ABS_RZ :
		  printf(" (Z Rate Axis) ");
		  break;
	      case ABS_THROTTLE :
		  printf(" (Throttle) ");
		  break;
	      case ABS_RUDDER :
		  printf(" (Rudder) ");
		  break;
	      case ABS_WHEEL :
		  printf(" (Wheel) ");
		  break;
	      case ABS_GAS :
		  printf(" (Accelerator) ");
		  break;
	      case ABS_BRAKE :
		  printf(" (Brake) ");
		  break;
	      case ABS_HAT0X :
		  printf(" (Hat zero, x axis) ");
		  break;
	      case ABS_HAT0Y :
		  printf(" (Hat zero, y axis) ");
		  break;
	      case ABS_HAT1X :
		  printf(" (Hat one, x axis) ");
		  break;
	      case ABS_HAT1Y :
		  printf(" (Hat one, y axis) ");
		  break;
	      case ABS_HAT2X :
		  printf(" (Hat two, x axis) ");
		  break;
	      case ABS_HAT2Y :
		  printf(" (Hat two, y axis) ");
		  break;
	      case ABS_HAT3X :
		  printf(" (Hat three, x axis) ");
		  break;
	      case ABS_HAT3Y :
		  printf(" (Hat three, y axis) ");
		  break;
	      case ABS_PRESSURE :
		  printf(" (Pressure) ");
		  break;
	      case ABS_DISTANCE :
		  printf(" (Distance) ");
		  break;
	      case ABS_TILT_X :
		  printf(" (Tilt, X axis) ");
		  break;
	      case ABS_TILT_Y :
		  printf(" (Tilt, Y axis) ");
		  break;
	      case ABS_MISC :
		  printf(" (Miscellaneous) ");
		  break;
	      default:
		  printf(" (Unknown absolute feature) ");
	      }
	  if(ioctl(fd, EVIOCGABS(axisindex), &abs_features)) {
	      perror("evdev EVIOCGABS ioctl");
              return 1;
	  }
          if ( deadzonevalue < abs_features.minimum || deadzonevalue > abs_features.maximum ){
	    printf("Deadzone value must be between %d and %d for this axis, value requested : %d\n",abs_features.minimum,abs_features.maximum,deadzonevalue);
          }
	  printf("Setting deadzone value to : %d\n",deadzonevalue);
          abs_features.flat=deadzonevalue;
	  if(ioctl(fd, EVIOCSABS(axisindex), &abs_features)) {
	      perror("evdev EVIOCSABS ioctl");
              return 1;
	  }
	  if(ioctl(fd, EVIOCGABS(axisindex), &abs_features)) {
	      perror("evdev EVIOCGABS ioctl");
              return 1;
	  }
	  pourcent_deadzone=(float)abs_features.flat*100/(float)abs_features.maximum;
	  printf("(min: %d, max: %d, flatness: %d (=%.2f%%), fuzz: %d)\n",
		 abs_features.minimum,
		 abs_features.maximum,
		 abs_features.flat,
                 pourcent_deadzone,
		 abs_features.fuzz);
      }
  }

  close(fd);
  return(0);
}

void help()
{
printf ("%s","\
Usage : \n\
\n\
Specific for the Logitech G25 wheel :\n\
\n\
G25manage [ --verbose|--brief ] [ --help ] [ --reconnect ] [ --nativemode ] [ --reset ] [ --range=degrees ]\n\
\n\
Dealing with calibration for all devices managed by the kernel event interface :\n\
\n\
  to see calibration informations : \n\
G25manage [ --showcalibration=/path/to/event/device/file ] \n\
\n\
  to set the deazone values :\n\
G25manage [ --evdev=/path/to/event/device/file --deadzone=deadzone_value [ --axis=axis_index ] ] \n\
\n\
Example :\n\
\n\
  I want to switch my G25 wheel to native mode to bee able to use all buttons and axes :\n\
G25manage --nativemode\n\
\n\
  I want to reset my G25 wheel to its default mode with default calibration values :\n\
G25manage --reset\n\
\n\
  I want to see the calibration values of my event managed joystick :\n\
G25manage --showcalibration=/dev/input/event6\n\
Supported Absolute axes:\n\
  Absolute axis 0x00 (0) (X Axis) (min: 0, max: 16383, flatness: 1023 (=6.24%), fuzz: 63)\n\
  Absolute axis 0x01 (1) (Y Axis) (min: 0, max: 255, flatness: 15 (=5.88%), fuzz: 0)\n\
  Absolute axis 0x02 (2) (Z Axis) (min: 0, max: 255, flatness: 15 (=5.88%), fuzz: 0)\n\
  Absolute axis 0x05 (5) (Z Rate Axis) (min: 0, max: 255, flatness: 15 (=5.88%), fuzz: 0)\n\
  Absolute axis 0x10 (16) (Hat zero, x axis) (min: -1, max: 1, flatness: 0 (=0.00%), fuzz: 0)\n\
  Absolute axis 0x11 (17) (Hat zero, y axis) (min: -1, max: 1, flatness: 0 (=0.00%), fuzz: 0)\n\
\n\
  I want to get rid of the deadzone on my wheel :\n\
G25manage --evdev=/dev/input/event6 --deadzone=0\n\
");

}

int main (argc, argv)
     int argc;
     char **argv;
{
  unsigned short int range;
  char *evdevice;
  int c;
  __s32 flat;
  int axisindex;
  bool setdeadzone;
  axisindex = 0;
  setdeadzone = false;
  evdevice = NULL;

  while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       &verbose_flag, 1},
          {"brief",   no_argument,       &verbose_flag, 0},
          /* These options don't set a flag.
             We distinguish them by their indices. */
          {"help",   no_argument,       0, 'h'},
          {"reconnect",  no_argument, 0, 'c'},
          {"nativemode",     no_argument,       0, 'n'},
          {"reset",  no_argument,       0, 'z'},
          {"range",  required_argument, 0, 'r'},
          {"evdev",  required_argument, 0, 'd'},
          {"deadzone",  required_argument, 0, 'f'},
          {"axis",  required_argument, 0, 'a'},
          {"showcalibration",  required_argument, 0, 's'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "hcnzr:d:f:a:s:",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 'a':
	  axisindex=atoi(optarg);
          printf ("Axis index to deal with: %d\n", axisindex);
          break;

        case 'h':
	  help();
          break;


        case 'c':
	  usb_rebind_kernel_driver_np();
          break;

        case 'd':
	  evdevice=optarg;
          printf ("Event device file: %s\n", evdevice);
          break;

        case 'f':
	  flat=atoi(optarg);
          setdeadzone=true;
          printf ("New dead zone value: %d\n", flat);
          break;

        case 'n':
	  G25native();
          break;

        case 'z':
	  G25reset();
          break;

        case 'r':
	  range=atoi(optarg);
	  G25range(range);
          break;

        case 's':
	  evdevice=optarg;
	  showcalibration(evdevice);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

  if (setdeadzone) {
    if (evdevice == NULL){
      printf ( "You must specify the event device for your joystick/wheel\n" );
      exit (1);
    }
    else {
      printf ( "Trying to set axis %d deadzone to: %d\n",axisindex,flat );
      setdeadzonevalue(evdevice,axisindex,flat);
    }
  }

  exit (0);
}
