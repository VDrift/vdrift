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

#include "linearinterp.h"
#include "unittest.h"

#include <cmath>

#include <iostream>
using std::ostream;

QT_TEST(linearinterp_test)
{
	{
		LinearInterp <float> l;
		QT_CHECK_CLOSE(l.Interpolate(1),0,0.0001);
	}

	{
		LinearInterp <float> l(3.1);
		QT_CHECK_CLOSE(l.Interpolate(1),3.1,0.0001);
	}

	{
		LinearInterp <float> l;
		l.AddPoint(2,1);
		QT_CHECK_CLOSE(l.Interpolate(1),1,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2),1,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3),1,0.0001);
	}

	{
		LinearInterp <float> l;
		l.AddPoint(2,1);
		l.AddPoint(3,2);

		l.SetBoundaryMode(LinearInterp<float>::CONSTANTSLOPE);

		QT_CHECK_CLOSE(l.Interpolate(1),0,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(0),-1,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2.5),1.5,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2.75),1.75,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3),2,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3.5),2.5,0.0001);
	}

	{
		LinearInterp <float> l;
		l.AddPoint(2,1);
		l.AddPoint(3,2);

		QT_CHECK_CLOSE(l.Interpolate(1),1,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(0),1,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2.5),1.5,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2.75),1.75,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3),2,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3.5),2,0.0001);
	}

	{
		LinearInterp <float> l;
		l.AddPoint(2,1);
		l.AddPoint(1,3);
		l.AddPoint(3,4);

		l.SetBoundaryMode(LinearInterp<float>::CONSTANTSLOPE);

		QT_CHECK_CLOSE(l.Interpolate(1),3,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(0),5,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(-1),7,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(2.5),2.5,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3),4,0.0001);
		QT_CHECK_CLOSE(l.Interpolate(3.5),5.5,0.0001);
	}
}
