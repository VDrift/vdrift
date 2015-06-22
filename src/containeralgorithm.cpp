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

#include "containeralgorithm.h"
#include "unittest.h"

#include <vector>
#include <string>
#include <iterator> // back_inserter

using std::vector;
using std::string;

namespace TESTONLY
{
class myclass
{
	public:
		myclass() : sum(0) {}
		int sum;
		void operator() (const std::string & s) {sum += s.length();}//std::cout << sum << ", " << s.length() << std::endl;}
};
bool starts_with_1(const std::string & s) {return (s[0] == '1');}
}

std::ostream & operator << (std::ostream &os, const vector <string> & v)
{
	for (size_t i = 0; i < v.size()-1; i++)
	{
		os << v[i] << ", ";
	}
	os << v[v.size()-1];// << std::endl;
	return os;
}

static int string_length(const std::string & str)
{
	return str.length();
}

QT_TEST(calgo_test)
{
	vector <string> vec;
	vec.push_back("1234");
	vec.push_back("4321");
	vec.push_back("133");
	QT_CHECK_EQUAL(&(*calgo::find(vec, "133")), &vec[2]);

	vector <string> vec2;
	//vec2.resize(vec.size());
	calgo::copy(vec, std::back_inserter(vec2));
	QT_CHECK(vec == vec2);

	TESTONLY::myclass myobject = calgo::for_each(vec, TESTONLY::myclass());
	QT_CHECK_EQUAL(myobject.sum, 11);

	vector <string> vec3;
	calgo::copy_if(vec, std::back_inserter(vec3), TESTONLY::starts_with_1);
	QT_CHECK_EQUAL(vec3.size(), 2);
	QT_CHECK_EQUAL(vec3[0], "1234");
	QT_CHECK_EQUAL(vec3[1], "133");

	vector <int> vec4(vec.size());
	calgo::transform(vec, vec4.begin(), string_length);
	QT_CHECK_EQUAL(vec4.size(), 3);
	QT_CHECK_EQUAL(vec4[0], 4);
	QT_CHECK_EQUAL(vec4[1], 4);
	QT_CHECK_EQUAL(vec4[2], 3);

	vector <string> vec5;
	vector <string> target;
	vector <unsigned int> todel;

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	calgo::SwapAndPop(vec5, todel);
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(0);
	target[0] = vec5[2];
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(1);
	target[1] = vec5[2];
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(2);
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(0);
	todel.push_back(1);
	target[0] = vec5[2];
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(1);
	todel.push_back(2);
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	calgo::copy(vec, std::back_inserter(target));
	todel.push_back(0);
	todel.push_back(2);
	target[0] = vec5[1];
	calgo::SwapAndPop(vec5, todel);
	target.pop_back();
	target.pop_back();
	QT_CHECK_EQUAL(vec5, target);

	vec5.clear();
	target.clear();
	todel.clear();
	calgo::copy(vec, std::back_inserter(vec5));
	todel.push_back(0);
	todel.push_back(1);
	todel.push_back(2);
	calgo::SwapAndPop(vec5, todel);
	QT_CHECK_EQUAL(vec5, target);
}
