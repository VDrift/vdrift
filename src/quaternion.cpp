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

#include "quaternion.h"
#include "unittest.h"

#include <iostream>
using std::ostream;

QT_TEST(quaternion_test)
{
	Quat test1;
	QT_CHECK_EQUAL(test1.x(),0.0);
	QT_CHECK_EQUAL(test1.y(),0.0);
	QT_CHECK_EQUAL(test1.z(),0.0);
	QT_CHECK_EQUAL(test1.w(),1.0);
	Quat test2;
	test2.LoadIdentity();
	QT_CHECK_EQUAL(test1,test2);
	test1.w() = 0.7;
	test1.Normalize();
	QT_CHECK_EQUAL(test1.Magnitude(),1.0);
	float mat[16];
	test1.GetMatrix4(mat);
	QT_CHECK_EQUAL(mat[0],1.0);
	QT_CHECK_EQUAL(mat[1],0.0);
	QT_CHECK_EQUAL(mat[2],0.0);
	QT_CHECK_EQUAL(mat[3],0.0);
	QT_CHECK_EQUAL(mat[4],0.0);
	QT_CHECK_EQUAL(mat[5],1.0);
	QT_CHECK_EQUAL(mat[6],0.0);
	QT_CHECK_EQUAL(mat[7],0.0);
	QT_CHECK_EQUAL(mat[8],0.0);
	QT_CHECK_EQUAL(mat[9],0.0);
	QT_CHECK_EQUAL(mat[10],1.0);
	QT_CHECK_EQUAL(mat[11],0.0);
	QT_CHECK_EQUAL(mat[12],0.0);
	QT_CHECK_EQUAL(mat[13],0.0);
	QT_CHECK_EQUAL(mat[14],0.0);
	QT_CHECK_EQUAL(mat[15],1.0);

	float vec[3];
	vec[0] = 0;
	vec[1] = 0;
	vec[2] = 1;
	test1.LoadIdentity();
	test1.Rotate(3.141593*0.5, 0.0, 1.0, 0.0);
	test1.RotateVector(vec);
	QT_CHECK_CLOSE(vec[0], 1.0, 0.001);
	QT_CHECK_CLOSE(vec[1], 0.0, 0.001);
	QT_CHECK_CLOSE(vec[2], 0.0, 0.001);
	//std::cout << vec[0] << "," << vec[1] << "," << vec[2] << std::endl;

	test2.LoadIdentity();
	test1.Rotate(3.141593*0.5, 0.0, 0.0, 1.0);
	QT_CHECK_CLOSE(test1.GetAngleBetween(test2),3.141593*0.5,0.001);

	test1.LoadIdentity();
	test1.Rotate(3.141593*0.75, 0.0, 1.0, 0.0);
	test2.LoadIdentity();
	test2.Rotate(3.141593*0.25, 0.0, 1.0, 0.0);

	vec[0] = 0;
	vec[1] = 0;
	vec[2] = 1;
	test1.QuatSlerp(test2, 0.5).RotateVector(vec);
	QT_CHECK_CLOSE(vec[0], 1.0, 0.001);
	QT_CHECK_CLOSE(vec[1], 0.0, 0.001);
	QT_CHECK_CLOSE(vec[2], 0.0, 0.001);
}
