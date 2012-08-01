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

#include "physics/rotationalframe.h"
#include "unittest.h"

#include <iostream>
using std::cout;
using std::endl;

QT_TEST(rotationalframe_test)
{
	{
		ROTATIONALFRAME <double> frame;
		//frame.SetInertia(1.0);
		QUATERNION <double> initorient;
		frame.SetOrientation(initorient);
		MATHVECTOR <double, 3> initv;
		initv.Set(0,0,0);
		frame.SetAngularVelocity(initv);
		MATHVECTOR <double, 3> torque;
		torque.Set(0,1,0);

		//integrate for 10 seconds
		for (int i = 0; i < 1000; i++)
		{
			frame.Integrate1(0.01);
			torque.Set(0,1,0);
			torque = torque - frame.GetAngularVelocity() * 10.0f;
			frame.ApplyTorque(torque);
			frame.Integrate2(0.01);
		}

		/*cout << "t = " << t << endl;
		cout << "Calculated Orientation: " << frame.GetOrientation() << endl;
		cout << "Calculated Velocity: " << frame.GetAngularVelocity() << endl;*/

		QT_CHECK_CLOSE(frame.GetAngularVelocity()[1], 0.1, 0.0001);
	}

	{
		ROTATIONALFRAME <double> frame;
		//frame.SetInertia(1.0);
		QUATERNION <double> initorient;
		frame.SetOrientation(initorient);
		MATHVECTOR <double, 3> initv;
		initv.Set(0,0,0);
		frame.SetAngularVelocity(initv);
		MATHVECTOR <double, 3> torque;
		torque.Set(0,1,0);
		MATRIX3 <double> inertia;
		inertia.Scale(0.1);
		frame.SetInertia(inertia);

		//integrate for 10 seconds
		for (int i = 0; i < 1000; i++)
		{
			frame.Integrate1(0.01);
			frame.ApplyTorque(torque);
			frame.Integrate2(0.01);
		}

		//cout << "t = " << t << endl;
		//cout << "Calculated Orientation: " << frame.GetOrientation() << endl;
		//cout << "Calculated Velocity: " << frame.GetAngularVelocity() << endl;

		//QT_CHECK_CLOSE(frame.GetAngularVelocity()[1], 0.1, 0.0001);
		QT_CHECK_CLOSE(frame.GetAngularVelocity()[1], 100., 0.0001);
	}
}
