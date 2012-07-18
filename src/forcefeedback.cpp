#include "forcefeedback.h"

#include <cstring>

#if SDL_VERSION_ATLEAST(2,0,0)

FORCEFEEDBACK::FORCEFEEDBACK(
	std::string device,
	std::ostream & error_output,
	std::ostream & info_output) :
	device_name(device),
	enabled(true),
	stop_and_play(false),
	lastforce(0),
	haptic(0),
	effect_id(-1)
{
	// Close haptic if already open.
	if (haptic)
		SDL_HapticClose(haptic);

	// Does the device support haptic?
	//if (!SDL_JoystickIsHaptic(joystick))
	//	return;

	// Try to create haptic device.
	//haptic = SDL_HapticOpenFromJoystick(joystick);

	// Does the device support haptic?
	int haptic_id = 0;
	int haptic_num = SDL_NumHaptics();/*
	while (haptic_id < haptic_num)
	{
		std::string haptic_name(SDL_HapticName(haptic_id++));
		if (haptic_name == device_name)
			break;
	}*/
	if (haptic_id == haptic_num)
		return;

	// Try to create haptic device.
	haptic = SDL_HapticOpen(haptic_id);
	if (!haptic)
	{
		error_output << "Failed to initialize force feedback device: " << SDL_GetError();
		return;
	}

	// Check for constant force support.
	unsigned int haptic_query = SDL_HapticQuery(haptic);
	if (!(haptic_query & SDL_HAPTIC_CONSTANT))
	{
		SDL_HapticClose(haptic);
		haptic = NULL;
		return;
	}

	// Create the effect.
	memset(&effect, 0, sizeof(SDL_HapticEffect) ); // 0 is safe default
	effect.type = SDL_HAPTIC_CONSTANT;
	effect.constant.direction.type = SDL_HAPTIC_CARTESIAN; // Using cartesian direction encoding.
	effect.constant.direction.dir[0] = 1; // X position
	effect.constant.direction.dir[1] = 0; // Y position
	effect.constant.length = 0xffff;
	effect.constant.delay = 0;
	effect.constant.button = 0;
	effect.constant.interval = 0;
	effect.constant.level = 0;
	effect.constant.attack_length = 0;
	effect.constant.attack_level = 0;
	effect.constant.fade_length = 0;
	effect.constant.fade_level = 0;

	// Upload the effect.
	effect_id = SDL_HapticNewEffect(haptic, &effect);
	if (effect_id == -1)
	{
		error_output << "Failed to initialize force feedback effect: " << SDL_GetError();
		return;
	}

	info_output << "Force feedback enabled." << std::endl;
}

FORCEFEEDBACK::~FORCEFEEDBACK()
{
	if (haptic)
		SDL_HapticClose(haptic);
}

void FORCEFEEDBACK::update(
	double force,
	double * position,
	double dt,
	std::ostream & error_output)
{
	if (!enabled || !haptic || (effect_id != -1))
		return;

	// Clamp force.
	if (force > 1.0) force = 1.0;
	if (force < -1.0) force = -1.0;

	// Low pass filter.
	lastforce = (lastforce + force) * 0.5;

	// Update effect.
	effect.constant.level = Sint16(lastforce * 32767.0);
	int new_effect_id = SDL_HapticUpdateEffect(haptic, effect_id, &effect);
	if (new_effect_id == -1)
	{
		//error_output << "Failed to update force feedback effect: " << SDL_GetError();
		//return;
	}
	else
	{
		effect_id = new_effect_id;
	}

	// Run effect.
	if (SDL_HapticRunEffect(haptic, effect_id, 1) == -1)
	{
		error_output << "Failed to run force feedback effect: " << SDL_GetError();
		return;
	}
}

#elif defined(linux) || defined(__linux)

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using std::string;
using std::endl;

//#define TEST_BIT(bit,bits) (((bits[bit>>5]>>(bit&0x1f))&1)!=0)

/* Number of bits for 1 unsigned char */
#define nBitsPerUchar          (sizeof(unsigned char) * 8)

/* Number of unsigned chars to contain a given number of bits */
#define nUcharsForNBits(nBits) ((((nBits)-1)/nBitsPerUchar)+1)

/* Index=Offset of given bit in 1 unsigned char */
#define bitOffsetInUchar(bit)  ((bit)%nBitsPerUchar)

/* Index=Offset of the unsigned char associated to the bit
   at the given index=offset */
#define ucharIndexForBit(bit)  ((bit)/nBitsPerUchar)

/* Value of an unsigned char with bit set at given index=offset */
#define ucharValueForBit(bit)  (((unsigned char)(1))<<bitOffsetInUchar(bit))

/* Test the bit with given index=offset in an unsigned char array */
#define testBit(bit, array)    ((array[ucharIndexForBit(bit)] >> bitOffsetInUchar(bit)) & 1)

FORCEFEEDBACK::FORCEFEEDBACK(
	string device,
	std::ostream & error_output,
	std::ostream & info_output) :
	device_name(device),
	enabled(true),
	stop_and_play(false),
	lastforce(0)
{
	unsigned char key_bits[1 + KEY_MAX/8/sizeof(unsigned char)];
	unsigned char abs_bits[1 + ABS_MAX/8/sizeof(unsigned char)];
	unsigned char ff_bits[1 + FF_MAX/8/sizeof(unsigned char)];

	struct input_event event;
	int valbuf[16];

	// Open event device with write permission
	device_handle = open(device_name.c_str(),O_RDWR|O_NONBLOCK);
	if (device_handle<0)
	{
		error_output << "force feedback: can not open " << device_name << " (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Which buttons has the device?
	memset(key_bits,0,sizeof(key_bits));
	if (ioctl(device_handle,EVIOCGBIT(EV_KEY,sizeof(key_bits)),key_bits)<0)
	{
		error_output << "force feedback: can not get key bits (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Which axes has the device?
	memset(abs_bits,0,sizeof(abs_bits));
	if (ioctl(device_handle,EVIOCGBIT(EV_ABS,sizeof(abs_bits)),abs_bits)<0)
	{
		error_output << "force feedback: can not get abs bits (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Now get some information about force feedback
	memset(ff_bits,0,sizeof(ff_bits));
	if (ioctl(device_handle,EVIOCGBIT(EV_FF ,sizeof(ff_bits)),ff_bits)<0)
	{
		error_output << "force feedback: can not get ff bits (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Which axis is the x-axis?
	if      (testBit(ABS_X    ,abs_bits)) axis_code=ABS_X;
	else if (testBit(ABS_RX   ,abs_bits)) axis_code=ABS_RX;
	else if (testBit(ABS_WHEEL,abs_bits)) axis_code=ABS_WHEEL;
	else
	{
		error_output << "force feedback: no suitable x-axis found [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// get axis value range
	if (ioctl(device_handle,EVIOCGABS(axis_code),valbuf)<0)
	{
		error_output << "force feedback: can not get axis value range (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}
	axis_min=valbuf[1];
	axis_max=valbuf[2];
	if (axis_min>=axis_max)
	{
		error_output << "force feedback: bad axis value range (" << axis_min << "," << axis_max << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// force feedback supported?
	if (!testBit(FF_CONSTANT,ff_bits))
	{
		error_output << "force feedback: device (or driver) has no force feedback support [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Switch off auto centering
	memset(&event,0,sizeof(event));
	event.type=EV_FF;
	event.code=FF_AUTOCENTER;
	event.value=0;
	if (write(device_handle,&event,sizeof(event))!=sizeof(event))
	{
		error_output << "force feedback: failed to disable auto centering (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	// Initialize constant force effect
	memset(&effect,0,sizeof(effect));
	effect.type=FF_CONSTANT;
	effect.id=-1;
	effect.trigger.button=0;
	effect.trigger.interval=0;
	effect.replay.length=0xffff;
	effect.replay.delay=0;
	effect.u.constant.level=0;
	effect.direction=0xC000;
	effect.u.constant.envelope.attack_length=0;
	effect.u.constant.envelope.attack_level=0;
	effect.u.constant.envelope.fade_length=0;
	effect.u.constant.envelope.fade_level=0;

	// Upload effect
	if (ioctl(device_handle,EVIOCSFF,&effect)==-1)
	{
		error_output << "force feedback: uploading effect failed (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	/* Start effect */
	memset(&event,0,sizeof(event));
	event.type=EV_FF;
	event.code=effect.id;
	event.value=1;
	if (write(device_handle,&event,sizeof(event))!=sizeof(event))
	{
		error_output << "force feedback: starting effect failed (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
		disable();
		return;
	}

	info_output << "Force feedback initialized successfully" << std::endl;
}

FORCEFEEDBACK::~FORCEFEEDBACK()
{
	// dtor
}

void FORCEFEEDBACK::update(
	double force,
	double * position,
	double dt,
	std::ostream & error_output)
{
	if ( !enabled )
		return;

	struct input_event event;

        // Delete effect
	if (stop_and_play && effect.id!=-1)
	{
		if (ioctl(device_handle,EVIOCRMFF,effect.id)==-1) {
			error_output << "force feedback: removing effect failed (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
			disable();
			return;
		}
		effect.id=-1;
	}

	// Clamp force
	if (force>1.0) force=1.0;
	if (force<-1.0) force=-1.0;

	// Low pass filter
	lastforce = (lastforce + force) * 0.5;

	effect.direction=0xC000;
	effect.u.constant.level=(short)(lastforce * 32767.0); /* only to be safe */
	effect.u.constant.envelope.attack_level=effect.u.constant.level;
	effect.u.constant.envelope.fade_level=effect.u.constant.level;

        // Upload effect
	if (ioctl(device_handle,EVIOCSFF,&effect)==-1)
	{
              //error_output << "error uploading effect" << endl;
                /* We do not exit here. Indeed, too frequent updates may be
				* refused, but that is not a fatal error */
	}
	else if (effect.id!=-1)
        // Start effect
        //if (stop_and_play && effect.id!=-1)
	{
		memset(&event,0,sizeof(event));
		event.type=EV_FF;
		event.code=effect.id;
		event.value=1;
		if (write(device_handle,&event,sizeof(event))!=sizeof(event))
		{
			error_output << "force feedback: re-starting effect failed (" << strerror(errno) << ") [" << __FILE__ << ":" << __LINE__ << "]" << endl;
			disable();
			return;
		}
	}

        // Get events
	while (read(device_handle,&event,sizeof(event))==sizeof(event))
	{
		if (event.type==EV_ABS && event.code==axis_code)
		{
			*position=((double)(((short)event.value)-axis_min))*2.0/(axis_max-axis_min)-1.0;
			if (*position>1.0) *position=1.0;
			else if (*position<-1.0) *position=-1.0;
		}
	}
}
#else

FORCEFEEDBACK::FORCEFEEDBACK(
	std::string device,
	std::ostream & error_output,
	std::ostream & info_output)
{
	// Constructor
}

FORCEFEEDBACK::~FORCEFEEDBACK()
{
	// Destructor
}

void FORCEFEEDBACK::update(
	double force,
	double * position,
	double dt,
	std::ostream & error_output)
{
	// No force feedback support
}

#endif

void FORCEFEEDBACK::disable()
{
	enabled = false;
}
