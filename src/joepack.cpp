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

#include "joepack.h"
#include "endian_utility.h"
#include "unittest.h"

#include <map>
#include <fstream>
#include <cassert>

using std::string;
using std::map;
using std::ios_base;

struct JoePack::Impl
{
	struct FatEntry
	{
		FatEntry() : offset(0), length(0) { }
		unsigned offset;
		unsigned length;
	};
	const std::string versionstr;
	std::map <std::string, FatEntry> fat;
	std::map <std::string, FatEntry>::iterator curfa;
	std::ifstream f;

	Impl();
	bool Load(const string & fn);
	void Close();
	void fclose();
	bool fopen(const string & fn);
	int fread(void * buffer, const unsigned size, const unsigned count);
};

JoePack::Impl::Impl() : versionstr("JPK01.00")
{
	curfa = fat.end();
}

bool JoePack::Impl::Load(const string & fn)
{
	Close();
	f.clear();
	f.open(fn.c_str(), ios_base::binary);
	if (f)
	{
		//load header
		char * versioncstr = new char[versionstr.length() + 1];
		f.read(versioncstr, versionstr.length());
		versioncstr[versionstr.length()] = '\0';
		string fversionstr = versioncstr;
		delete [] versioncstr;
		if (fversionstr != versionstr)
		{
			//write out an error?
			return false;
		}

		unsigned int numobjs = 0;
		assert(sizeof(unsigned int) == 4);
		f.read((char*)(&numobjs), sizeof(unsigned int));
		numobjs = ENDIAN_SWAP_32(numobjs);

		//DPRINT(numobjs << " objects");

		unsigned int maxstrlen = 0;
		f.read((char*)(&maxstrlen), sizeof(unsigned int));
		maxstrlen = ENDIAN_SWAP_32(maxstrlen);

		//DPRINT(maxstrlen << " max string length");

		char * fnch = new char[maxstrlen+1];

		//load FAT
		for (unsigned int i = 0; i < numobjs; i++)
		{
			FatEntry fa;
			f.read((char*)(&(fa.offset)), sizeof(unsigned int));
			fa.offset = ENDIAN_SWAP_32(fa.offset);
			f.read((char*)(&(fa.length)), sizeof(unsigned int));
			fa.length = ENDIAN_SWAP_32(fa.length);
			f.read(fnch, maxstrlen);
			fnch[maxstrlen] = '\0';
			string filename = fnch;
			fat[filename] = fa;

			//DPRINT(filename << ": offest " << fa.offset << " length " << fa.length);
		}

		delete [] fnch;
		return true;
	}
	else
	{
		//write an error?
		return false;
	}
}

void JoePack::Impl::Close()
{
	if (f.is_open()) f.close();
	fat.clear();
	curfa = fat.end();
}

void JoePack::Impl::fclose()
{
	curfa = fat.end();
}

bool JoePack::Impl::fopen(const string & fn)
{
	curfa = fat.find(fn);
	if (curfa == fat.end())
	{
		return false;
	}
	else
	{
		f.seekg(curfa->second.offset);
		return true;
	}
}

int JoePack::Impl::fread(void * buffer, const unsigned size, const unsigned count)
{
	if (curfa != fat.end())
	{
		unsigned int abspos = f.tellg();
		assert(abspos >= curfa->second.offset);
		unsigned int relpos = abspos - curfa->second.offset;
		assert(curfa->second.length >= relpos);
		unsigned int fileleft = curfa->second.length - relpos;
		unsigned int requestedread = size*count;

		assert(size != 0);

		if (requestedread > fileleft)
		{
			//overflow
			requestedread = fileleft/size;
		}

		//DPRINT("JOEPACK fread: " << abspos << "," << relpos << "," << fileleft << "," << requestedread);
		f.read((char *)buffer, requestedread);
		return f.gcount()/size;
	}
	else
	{
		//write error?
		return 0;
	}
}

JoePack::JoePack()
{
	impl = new Impl();
}

JoePack::~JoePack()
{
	Close();
	delete impl;
}

bool JoePack::Load(const std::string & fn)
{
	packpath = fn;
	return impl->Load(fn);
}

void JoePack::Close()
{
	packpath.clear();
	impl->Close();
}

void JoePack::fclose() const
{
	impl->fclose();
}

bool JoePack::fopen(const string & fn) const
{
	string newfn;
	if (fn.find(packpath, 0) < fn.length())
	{
		newfn = fn.substr(packpath.length()+1);
	}
	else
	{
		newfn = fn;
	}
	return impl->fopen(newfn);
}

int JoePack::fread(void * buffer, const unsigned size, const unsigned count) const
{
	return impl->fread(buffer, size, count);
}

QT_TEST(joepack_test)
{
	JoePack p;
	QT_CHECK(p.Load("data/test/test1.jpk"));
	QT_CHECK(p.fopen("testlist.txt"));
	char buf[1000];
	unsigned int chars = p.fread(buf, 1, 999);
	QT_CHECK_EQUAL(chars, 16);
	buf[chars] = '\0';
	string comparisonstr = "This is\na test.\n";
	string filestr = buf;
	QT_CHECK_EQUAL(buf,comparisonstr);
}
