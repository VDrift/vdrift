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

#include "utils.h"

#include "unittest.h"

#include <fstream>
#include <cassert>

namespace Utils
{

std::string LoadFileIntoString(const std::string & filepath, std::ostream & error_output)
{
	std::string filestring;

	std::ifstream f;
	f.open(filepath.c_str());
	if (f)
	{
		char c[1024];

		while (f.good())
		{
			f.get(c, 1024, 0);
			filestring = filestring + c;
		}

		f.close();
	}
	else
	{
		error_output << "File not found: " + filepath << std::endl;
	}

	return filestring;
}

std::string SeekTo(std::istream & in, const std::string & token)
{
	std::string sofar; // what we've read in so far
	std::string potential; // potential token

	while (in && potential != token)
	{
		char c = in.get();
		if (in)
		{
			//std::cout << "read char: " << (int)c << std::endl;
			assert(token.size() >= potential.size());
			if (token[potential.size()] == c)
			{
				//std::cout << "adding to potential" << std::endl;
				potential.push_back(c);
			}
			else
			{
				//std::cout << "adding to sofar" << std::endl;
				sofar.append(potential);
				potential.clear();
				sofar.push_back(c);
			}
		}
	}

	//std::cout << "sofar: " << sofar << ", potential: " << potential << std::endl;

	if (potential != token)
		return sofar+potential;
	else
		return sofar;
}

QT_TEST(utils)
{
	std::string res;
	{
		std::istringstream s("testing 123");
		res = SeekTo(s,"1");
		QT_CHECK_EQUAL(res,"testing ");
		res = SeekTo(s,"2");
		QT_CHECK_EQUAL(res,"");
		res = SeekTo(s,"2");
		QT_CHECK_EQUAL(res,"3");
	}
	{
		std::istringstream s("testing 123");
		res = SeekTo(s,"test");
		QT_CHECK_EQUAL(res,"");
		res = SeekTo(s," ");
		QT_CHECK_EQUAL(res,"ing");
		res = SeekTo(s,"123");
		QT_CHECK_EQUAL(res,"");
	}
	{
		std::istringstream s("testing 123");
		res = SeekTo(s,"esting");
		QT_CHECK_EQUAL(res,"t");
		res = SeekTo(s,"2");
		QT_CHECK_EQUAL(res," 1");
		res = SeekTo(s,"3");
		QT_CHECK_EQUAL(res,"");
	}
	{
		std::istringstream s("testing 123");
		res = SeekTo(s," ");
		QT_CHECK_EQUAL(res,"testing");
		res = SeekTo(s," ");
		QT_CHECK_EQUAL(res,"123");
	}
}

std::vector <std::string> explode(const std::string & toExplode, const std::string & sep)
{
	std::vector <std::string> out;
	size_t lastPos = 0;
	size_t curPos = 0;

	curPos = toExplode.find(sep, lastPos);
	while (curPos != std::string::npos)
	{
		out.push_back(toExplode.substr(lastPos, curPos-lastPos));
		lastPos = curPos+sep.size();
		curPos = toExplode.find(sep, lastPos);
	}

	if (lastPos != curPos && lastPos < toExplode.size())
		out.push_back(toExplode.substr(lastPos));
	else
		out.push_back("");

	return out;
}

}

QT_TEST(utils_test)
{
	{
		std::vector <std::string> exploded = Utils::explode("/home/joe/code/test.hog", "/");
		QT_CHECK_EQUAL(exploded.size(), 5);
		if (exploded.size() > 0) QT_CHECK_EQUAL(exploded[0], "");
		if (exploded.size() > 1) QT_CHECK_EQUAL(exploded[1], "home");
		if (exploded.size() > 2) QT_CHECK_EQUAL(exploded[2], "joe");
		if (exploded.size() > 3) QT_CHECK_EQUAL(exploded[3], "code");
		if (exploded.size() > 4) QT_CHECK_EQUAL(exploded[4], "test.hog");
	}

	{
		std::vector <std::string> exploded = Utils::explode("/home/joe/code/", "/");
		QT_CHECK_EQUAL(exploded.size(), 5);
		if (exploded.size() > 0) QT_CHECK_EQUAL(exploded[0], "");
		if (exploded.size() > 1) QT_CHECK_EQUAL(exploded[1], "home");
		if (exploded.size() > 2) QT_CHECK_EQUAL(exploded[2], "joe");
		if (exploded.size() > 3) QT_CHECK_EQUAL(exploded[3], "code");
		if (exploded.size() > 4) QT_CHECK_EQUAL(exploded[4], "");
	}

	{
		std::vector <std::string> exploded = Utils::explode("c:/home/joe/code/test.hog", "/");
		QT_CHECK_EQUAL(exploded.size(), 5);
		if (exploded.size() > 0) QT_CHECK_EQUAL(exploded[0], "c:");
		if (exploded.size() > 1) QT_CHECK_EQUAL(exploded[1], "home");
		if (exploded.size() > 2) QT_CHECK_EQUAL(exploded[2], "joe");
		if (exploded.size() > 3) QT_CHECK_EQUAL(exploded[3], "code");
		if (exploded.size() > 4) QT_CHECK_EQUAL(exploded[4], "test.hog");
	}
}
