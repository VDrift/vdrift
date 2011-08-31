#ifndef _JOEPACK_H
#define _JOEPACK_H

#include <string>

class JOEPACK
{
public:
	JOEPACK();

	~JOEPACK();

	const std::string & GetPath() const {return packpath;}

	bool Load(const std::string & fn);

	void Close();

	bool fopen(const std::string & fn) const;

	void fclose() const;

	int fread(void * buffer, const unsigned size, const unsigned count) const;

private:
	std::string packpath;
	struct IMPL;
	IMPL* impl;
};

#endif
