#include "containeralgorithm.h"
#include "unittest.h"

#include <vector>
#include <string>

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
};

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
	calgo::transform(vec, vec4.begin(), std::mem_fun_ref(&string::length));
	QT_CHECK_EQUAL(vec4.size(), 3);
	QT_CHECK_EQUAL(vec4[0], 4);
	QT_CHECK_EQUAL(vec4[1], 4);
	QT_CHECK_EQUAL(vec4[2], 3);
}
