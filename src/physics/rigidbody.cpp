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

#include "physics/rigidbody.h"
#include "unittest.h"

#include <iostream>
using std::cout;
using std::endl;

QT_TEST(rigidbody_test)
{
	RIGIDBODY <float> body;
	MATHVECTOR <float, 3> initpos;
	QUATERNION <float> quat;
	initpos.Set(0,0,10);
	body.SetPosition(initpos);
	quat.Rotate(-3.141593*0.5, 1, 0, 0);
	body.SetOrientation(quat);

	MATHVECTOR <float, 3> localcoords;
	localcoords.Set(0,0,1);
	MATHVECTOR <float, 3> expected;
	expected.Set(0,1,10);
	MATHVECTOR <float, 3> pos = body.TransformLocalToWorld(localcoords);
	QT_CHECK_CLOSE(pos[0], expected[0], 0.0001);
	QT_CHECK_CLOSE(pos[1], expected[1], 0.0001);
	QT_CHECK_CLOSE(pos[2], expected[2], 0.0001);

	QT_CHECK_CLOSE(body.TransformWorldToLocal(pos)[0], localcoords[0], 0.0001);
	QT_CHECK_CLOSE(body.TransformWorldToLocal(pos)[1], localcoords[1], 0.0001);
	QT_CHECK_CLOSE(body.TransformWorldToLocal(pos)[2], localcoords[2], 0.0001);
}
