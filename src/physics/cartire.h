#ifndef _CARTIRE_H
#define _CARTIRE_H

//#define VDRIFTP

#if defined(VDRIFTP)
	#include "physics/cartire3.h"
	typedef CarTire3 CarTire;
	typedef CarTireInfo3 CarTireInfo;
#elif defined(VDRIFTN)
	#include "physics/cartire2.h"
	typedef CarTire2 CarTire;
	typedef CarTireInfo2 CarTireInfo;
#else
	#include "physics/cartire1.h"
	typedef CarTire1 CarTire;
	typedef CarTireInfo1 CarTireInfo;
#endif

#endif
