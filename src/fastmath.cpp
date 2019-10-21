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

#include "fastmath.h"
#include "unittest.h"

QT_TEST(TanPi4)
{
	QT_CHECK_CLOSE(std::tan(0), TanPi4(0), FM_TAN_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = (M_PI_4 * 0.1) * i;
		QT_CHECK_CLOSE(std::tan(x), TanPi4(x), FM_TAN_ERROR);
		QT_CHECK_CLOSE(std::tan(-x), TanPi4(-x), FM_TAN_ERROR);
	}
}

QT_TEST(TanPi2)
{
	QT_CHECK_CLOSE(std::tan(0), TanPi2(0), FM_TAN_ERROR);
	for (int i = 1; i < 10; i++)
	{
		float x = (M_PI_2 * 0.1) * i;
		QT_CHECK_CLOSE(std::tan(x), TanPi2(x), FM_TAN_ERROR);
		QT_CHECK_CLOSE(std::tan(-x), TanPi2(-x), FM_TAN_ERROR);
	}
	QT_CHECK_CLOSE(std::tan(M_PI_2 * 0.99), TanPi2(M_PI_2 * 0.99), 8E-4);
}

QT_TEST(Atan1)
{
	QT_CHECK_CLOSE(std::atan(0), Atan1(0), FM_ATAN_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = 0.1 * i;
		QT_CHECK_CLOSE(std::atan(x), Atan1(x), FM_ATAN_ERROR);
		QT_CHECK_CLOSE(std::atan(-x), Atan1(-x), FM_ATAN_ERROR);
	}
}

QT_TEST(Atan)
{
	QT_CHECK_CLOSE(std::atan(0), Atan(0), FM_ATAN_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = 0.2 * i;
		QT_CHECK_CLOSE(std::atan(x), Atan(x), FM_ATAN_ERROR);
		QT_CHECK_CLOSE(std::atan(-x), Atan(-x), FM_ATAN_ERROR);
	}
}

QT_TEST(CosPi2)
{
	QT_CHECK_CLOSE(std::cos(0), CosPi2(0), FM_COS_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = (M_PI_2 * 0.1) * i;
		QT_CHECK_CLOSE(std::cos(x), CosPi2(x), FM_COS_ERROR);
		QT_CHECK_CLOSE(std::cos(-x), CosPi2(-x), FM_COS_ERROR);
	}
}

QT_TEST(Cos3Pi2)
{
	QT_CHECK_CLOSE(std::cos(0), Cos3Pi2(0), FM_COS_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = (3 * M_PI_2 * 0.1) * i;
		QT_CHECK_CLOSE(std::cos(x), Cos3Pi2(x), FM_COS_ERROR);
		QT_CHECK_CLOSE(std::cos(-x), Cos3Pi2(-x), FM_COS_ERROR);
	}
}

QT_TEST(SinPi2)
{
	QT_CHECK_CLOSE(std::sin(0), SinPi2(0), FM_SIN_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = (M_PI_2 * 0.1) * i;
		QT_CHECK_CLOSE(std::sin(x), SinPi2(x), FM_SIN_ERROR);
		QT_CHECK_CLOSE(std::sin(-x), SinPi2(-x), FM_SIN_ERROR);
	}
}

QT_TEST(Sin3Pi2)
{
	QT_CHECK_CLOSE(std::sin(0), Sin3Pi2(0), FM_SIN_ERROR);
	for (int i = 1; i < 11; i++)
	{
		float x = (3 * M_PI_2 * 0.1) * i;
		QT_CHECK_CLOSE(std::sin(x), Sin3Pi2(x), FM_SIN_ERROR);
		QT_CHECK_CLOSE(std::sin(-x), Sin3Pi2(-x), FM_SIN_ERROR);
	}
}
