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

#include "keyed_container.h"
#include "unittest.h"
#include "quickprof.h"

#include <stdint.h>

QT_TEST(keyed_container_test)
{
	keyed_container <int> data;
	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());

	keyed_container <int>::handle handle1 = data.insert(1);
	keyed_container <int>::handle handle2 = data.insert(2);
	keyed_container <int>::handle handle3 = data.insert(3);

	QT_CHECK(!(handle1 == handle2));
	QT_CHECK(!(handle1 == handle3));
	QT_CHECK(!(handle3 == handle2));
	QT_CHECK_EQUAL(data.size(), 3);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle2), 2);
	QT_CHECK_EQUAL(data.get(handle3), 3);

	QT_CHECK_EQUAL(*data.find(handle2), 2);

	data.erase(handle2);
	data.erase(handle1);

	QT_CHECK_EQUAL(data.size(), 1);
	QT_CHECK(!data.empty());
	QT_CHECK(!data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK(data.find(handle1) == data.end());
	QT_CHECK_EQUAL(data.get(handle3), 3);
	QT_CHECK_EQUAL(*data.find(handle3), 3);

	int count = 0;
	for (keyed_container <int>::iterator i = data.begin(); i != data.end(); i++)
	{
		QT_CHECK_EQUAL(*i, 3);

		count++;
	}
	QT_CHECK_EQUAL(count, 1);

	handle1 = data.insert(1);

	QT_CHECK_EQUAL(data.size(), 2);
	QT_CHECK(!data.empty());
	QT_CHECK(data.contains(handle1));
	QT_CHECK(!data.contains(handle2));
	QT_CHECK(data.contains(handle3));
	QT_CHECK(data.find(handle2) == data.end());
	QT_CHECK_EQUAL(data.get(handle1), 1);
	QT_CHECK_EQUAL(data.get(handle3), 3);
	QT_CHECK_EQUAL(*data.find(handle3), 3);

	data.clear();

	QT_CHECK(data.empty());
	QT_CHECK_EQUAL(data.size(), 0);
	QT_CHECK(data.begin() == data.end());
}
