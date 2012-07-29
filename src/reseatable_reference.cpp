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

#include "reseatable_reference.h"
#include "unittest.h"

QT_TEST(reseatable_reference_test)
{
	int i1 = 1337;

	reseatable_reference <int> r1;
	QT_CHECK(!r1);
	r1 = i1;
	QT_CHECK(r1);
	QT_CHECK_EQUAL(r1.get(), 1337);
	QT_CHECK_EQUAL(*r1, 1337);

	reseatable_reference <int> r2 = i1;
	QT_CHECK(r2);
	QT_CHECK_EQUAL(r2.get(), 1337);

	int & ref = i1;

	reseatable_reference <int> r3 = ref;
	QT_CHECK(r3);
	QT_CHECK_EQUAL(r3.get(), 1337);
}
