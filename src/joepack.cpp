#include "joepack.h"
#include "endian_utility.h"
#include "unittest.h"

#include <map>
#include <fstream>
#include <cassert>

using std::string;
using std::map;
using std::ios_base;

struct JOEPACK::IMPL
{
	struct FADATA
	{
		FADATA() : offset(0), length(0) { }
		unsigned offset;
		unsigned length;
	};
	const std::string versionstr;
	std::map <std::string, FADATA> fat;
	std::map <std::string, FADATA>::iterator curfa;
	std::ifstream f;

	IMPL();
	bool Load(const string & fn);
	void Close();
	void fclose();
	bool fopen(const string & fn);
	int fread(void * buffer, const unsigned size, const unsigned count);
};

JOEPACK::IMPL::IMPL() : versionstr("JPK01.00")
{
	curfa = fat.end();
}

bool JOEPACK::IMPL::Load(const string & fn)
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
			FADATA fa;
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

void JOEPACK::IMPL::Close()
{
	if (f.is_open()) f.close();
	fat.clear();
	curfa = fat.end();
}

void JOEPACK::IMPL::fclose()
{
	curfa = fat.end();
}

bool JOEPACK::IMPL::fopen(const string & fn)
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

int JOEPACK::IMPL::fread(void * buffer, const unsigned size, const unsigned count)
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

JOEPACK::JOEPACK()
{
	impl = new IMPL();
}

JOEPACK::~JOEPACK()
{
	Close();
	delete impl;
}

bool JOEPACK::Load(const std::string & fn)
{
	packpath = fn;
	return impl->Load(fn);
}

void JOEPACK::Close()
{
	packpath.clear();
	impl->Close();
}

void JOEPACK::fclose() const
{
	impl->fclose();
}

bool JOEPACK::fopen(const string & fn) const
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

int JOEPACK::fread(void * buffer, const unsigned size, const unsigned count) const
{
	return impl->fread(buffer, size, count);
}

QT_TEST(joepack_test)
{
	JOEPACK p;
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
