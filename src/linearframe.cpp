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

#include "linearframe.h"
#include "unittest.h"

#include <iostream>
using std::cout;
using std::endl;

QT_TEST(linearframe_test)
{
	LINEARFRAME <double> frame;
	frame.SetMass(1.0);
	MATHVECTOR <double, 3> initpos;
	initpos.Set(0,0,0);
	frame.SetPosition(initpos);
	MATHVECTOR <double, 3> initv;
	initv.Set(0,65,0);
	frame.SetVelocity(initv);
	MATHVECTOR <double, 3> gravity;
	gravity.Set(0,-9.81,0);

	double t = 0.0;

	//integrate for 10 seconds
	for (int i = 0; i < 1000; i++)
	{
		frame.Integrate1(0.01);
		frame.ApplyForce(gravity);
		frame.Integrate2(0.01);
		t += 0.01;
	}

	/*cout << "t = " << t << endl;
	cout << "Calculated Position: " << frame.GetPosition() << endl;
	//cout << "Velocity: " << frame.GetVelocity() << endl;
	cout << "Expected Position: " << initv * t + gravity * t * t *0.5 << endl;*/

	QT_CHECK_CLOSE(frame.GetPosition()[1], (initv * t + gravity * t * t *0.5)[1], 0.0001);


	frame.SetMass(1.0);
	initpos.Set(0,0,0);
	frame.SetPosition(initpos);
	initv.Set(0,0,0);
	frame.SetVelocity(initv);
	MATHVECTOR <double, 3> force;

	t = 0.0;

	//integrate for 10 seconds
	for (int i = 0; i < 1000; i++)
	{
		frame.Integrate1(0.01);
		force.Set(0,1,0);
		force = force - frame.GetVelocity() * 10.0f;
		frame.ApplyForce(force);
		frame.Integrate2(0.01);
		t += 0.01;
	}

	//cout << "Velocity: " << frame.GetVelocity() << endl;

	QT_CHECK_CLOSE(frame.GetVelocity()[1], 0.1, 0.0001);
}
