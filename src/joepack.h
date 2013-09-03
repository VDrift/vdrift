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

#ifndef _JOEPACK_H
#define _JOEPACK_H

#include <string>

class JoePack
{
public:
	JoePack();

	~JoePack();

	const std::string & GetPath() const {return packpath;}

	bool Load(const std::string & fn);

	void Close();

	bool fopen(const std::string & fn) const;

	void fclose() const;

	int fread(void * buffer, const unsigned size, const unsigned count) const;

private:
	std::string packpath;
	struct Impl;
	Impl* impl;
};

#endif
