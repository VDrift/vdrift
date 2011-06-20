#ifndef _COORDINATESYSTEM_H
#define _COORDINATESYSTEM_H

#include "LinearMath/btVector3.h"
#include "mathvector.h"

namespace direction {

static const btVector3 right(1, 0, 0);
static const btVector3 forward(0, 1, 0);
static const btVector3 up(0, 0, 1);

static const MATHVECTOR<float, 3> Right(1, 0, 0);
static const MATHVECTOR<float, 3> Forward(0, 1, 0);
static const MATHVECTOR<float, 3> Up(0, 0, 1);

}

#endif //_COORDINATESYSTEM_H
