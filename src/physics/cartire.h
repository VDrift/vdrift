#ifndef _CARTIRE_H
#define _CARTIRE_H

#if defined(VDRIFTN)
	#include "physics/cartire2.h"
	typedef CarTire2 CarTire;
	typedef CarTireInfo2 CarTireInfo;
#else
	#include "physics/cartire1.h"
	typedef CarTire1 CarTire;
	typedef CarTireInfo1 CarTireInfo;
#endif

#endif
