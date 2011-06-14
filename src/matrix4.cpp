#include "matrix4.h"
#include "mathvector.h"
#include "quaternion.h"
#include "unittest.h"

QT_TEST(matrix4_test)
{
	QUATERNION <float> quat;
	MATHVECTOR <float, 3> vec;
	MATRIX4 <float> mat;

	vec.Set(0,10,0);
	quat.Rotate(3.141593*0.5,0,1,0);

	quat.GetMatrix4(mat);
	mat.Translate(vec[0], vec[1], vec[2]);

	MATHVECTOR <float, 3> out(0,0,1);
	MATHVECTOR <float, 3> orig = out;
	mat.TransformVectorOut(out[0], out[1], out[2]);

	MATHVECTOR <float, 3> comp(orig);
	quat.RotateVector(comp);
	comp = comp + vec;

	QT_CHECK_CLOSE(out[0], comp[0], 0.001);
	QT_CHECK_CLOSE(out[1], comp[1], 0.001);
	QT_CHECK_CLOSE(out[2], comp[2], 0.001);

	MATHVECTOR <float, 3> in(out);
	mat.TransformVectorIn(in[0], in[1], in[2]);

	QT_CHECK_CLOSE(in[0], orig[0], 0.001);
	QT_CHECK_CLOSE(in[1], orig[1], 0.001);
	QT_CHECK_CLOSE(in[2], orig[2], 0.001);
}
