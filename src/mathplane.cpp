/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "mathplane.h"
#include "unittest.h"

#include <iostream>
using std::ostream;

QT_TEST(mathplane_test)
{
	QT_CHECK_CLOSE(MATHPLANE <float> (0,1,0,0).DistanceToPoint(MATHVECTOR <float, 3> (0,0,0)), 0.0f, 0.0001f);
	QT_CHECK_CLOSE(MATHPLANE <float> (0,1,0,0).DistanceToPoint(MATHVECTOR <float, 3> (1,1,1)), 1.0f, 0.0001f);
	QT_CHECK_CLOSE(MATHPLANE <float> (0,1,0,0).DistanceToPoint(MATHVECTOR <float, 3> (1,-1,1)), -1.0f, 0.0001f);
	QT_CHECK_CLOSE(MATHPLANE <float> (0,1,0,-3).DistanceToPoint(MATHVECTOR <float, 3> (100,3,-40)), 0.0f, 0.0001f);
}
