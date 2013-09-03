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

#include "mathvector.h"
#include "unittest.h"

#include <iostream>
using std::ostream;

QT_TEST(mathvector_test)
{
	{
		MathVector <int, 1> test1(8);
		MathVector <int, 1> test2(2);
		test2.Set(5);
		MathVector <int, 1> test3;
		test3 = test1 + test2;
		QT_CHECK_EQUAL(test3[0], 13);
	}

	{
		Vec2 test1(0);
		test1.Set(3,4);
		Vec2 test2(1);
		Vec2 test3 (test1-test2);
		QT_CHECK_EQUAL(test3[0], 2);
		QT_CHECK_EQUAL(test3[1], 3);
	}

	{
		Vec3 test1;
		test1.Set(1,2,3);
		Vec3 test2(1);
		Vec3 test3 (test1+test2);
		QT_CHECK_EQUAL(test3[0], 2);
		QT_CHECK_EQUAL(test3[1], 3);
		QT_CHECK_EQUAL(test3[2], 4);

		test3 = test1;
		QT_CHECK(test1 == test3);
		QT_CHECK(test1 != test2);

		test3 = -test1;
		Vec3 answer;
		answer.Set(-1,-2,-3);
		QT_CHECK_EQUAL(test3,answer);
	}

	{
		Vec3 test1;
		test1.Set(1,2,3);
		Vec3 testcopy(test1);
		QT_CHECK_EQUAL(test1,testcopy);
		float v3[3];
		for (int i = 0; i < 3; i++)
			v3[i] = i + 1;
		testcopy.Set(v3);
		QT_CHECK_EQUAL(test1,testcopy);
		Vec3 add1;
		add1.Set(1,1,1);
		for (int i = 0; i < 3; i++)
			testcopy[i] = i;
		testcopy = testcopy + add1;
		QT_CHECK_EQUAL(test1,testcopy);
		for (int i = 0; i < 3; i++)
			testcopy[i] = i+2;
		testcopy = testcopy - add1;
		QT_CHECK_EQUAL(test1,testcopy);
		testcopy = testcopy * 1.0;
		QT_CHECK_EQUAL(test1,testcopy);
		testcopy = testcopy / 1.0;
		QT_CHECK_EQUAL(test1,testcopy);
		QT_CHECK(test1 == testcopy);
		QT_CHECK(!(test1 == add1));
		QT_CHECK(test1 != add1);
		QT_CHECK(!(test1 != testcopy));
		for (int i = 0; i < 3; i++)
			testcopy[i] = -(i+1);
		testcopy = -testcopy;
		Vec3 zero(0);
		for (int i = 0; i < 3; i++)
			QT_CHECK_EQUAL(zero[i],0);
		QT_CHECK_EQUAL(test1,testcopy);
		testcopy.Set(0.0);
		QT_CHECK_EQUAL(testcopy,zero);
		testcopy = test1;
		QT_CHECK_EQUAL(testcopy,test1);
		QT_CHECK_CLOSE(test1.MagnitudeSquared(),14.0,0.001);
		QT_CHECK_CLOSE(test1.Magnitude(),3.741657,0.001);

		QT_CHECK_CLOSE(test1.Normalize()[0],0.267261,0.001);
		QT_CHECK_CLOSE(test1.Normalize()[1],0.534522,0.001);

		Vec3 test2;
		for (int i = 0; i < 3; i++)
			QT_CHECK_EQUAL(test2[i],0);
		test2.Set(2,3,4);
		QT_CHECK_CLOSE(test1.dot(test2),20.0,0.001);

		test1.Set(1,-1,-2);
		test1.absify();
		QT_CHECK_EQUAL(test1,(Vec3(1,1,2)));
	}

	{
		Vec3 test1;
		test1.Set(1,2,3);
		Vec3 test2;
		test2.Set(4,5,6);
		Vec3 answer;
		answer.Set(-3,6,-3);
		QT_CHECK_EQUAL(test1.cross(test2), answer);
	}
}
