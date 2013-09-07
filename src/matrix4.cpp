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

#include "matrix4.h"
#include "mathvector.h"
#include "quaternion.h"
#include "unittest.h"

QT_TEST(matrix4_test)
{
	Quat quat;
	Vec3 vec;
	Mat4 mat;

	vec.Set(0,10,0);
	quat.Rotate(3.141593*0.5,0,1,0);

	quat.GetMatrix4(mat);
	mat.Translate(vec[0], vec[1], vec[2]);

	Vec3 out(0,0,1);
	Vec3 orig = out;
	mat.TransformVectorOut(out[0], out[1], out[2]);

	Vec3 comp(orig);
	quat.RotateVector(comp);
	comp = comp + vec;

	QT_CHECK_CLOSE(out[0], comp[0], 0.001);
	QT_CHECK_CLOSE(out[1], comp[1], 0.001);
	QT_CHECK_CLOSE(out[2], comp[2], 0.001);

	Vec3 in(out);
	mat.TransformVectorIn(in[0], in[1], in[2]);

	QT_CHECK_CLOSE(in[0], orig[0], 0.001);
	QT_CHECK_CLOSE(in[1], orig[1], 0.001);
	QT_CHECK_CLOSE(in[2], orig[2], 0.001);
}
