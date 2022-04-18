#ifndef _CARTIRE_H
#define _CARTIRE_H

#include "cartirebase.h"

//#define VDRIFTP

#if defined(VDRIFTP)
	#include "physics/cartire3.h"
	using CarTire = CarTire3;
#elif defined(VDRIFTN)
	#include "physics/cartire2.h"
	using CarTire = CarTire2;
#else
	#include "physics/cartire1.h"
	using CarTire = CarTire1;
#endif

#endif
