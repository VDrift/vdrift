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

#ifndef _SOUNDFACTORY_H
#define _SOUNDFACTORY_H

#include "content/contentfactory.h"
#include "sound/soundinfo.h"

class SOUNDBUFFER;

template <>
class Factory<SOUNDBUFFER>
{
public:
	struct empty {};

	Factory();

	/// sound device setting
	void init(const SOUNDINFO& value);

	template <class P>
	bool create(
		std::tr1::shared_ptr<SOUNDBUFFER> & sptr,
		std::ostream & error,
		const std::string & basepath,
		const std::string & path,
		const std::string & name,
		const P & param);

	std::tr1::shared_ptr<SOUNDBUFFER> getDefault() const;

private:
	std::tr1::shared_ptr<SOUNDBUFFER> m_default;
	SOUNDINFO m_info;
};

#endif // _SOUNDFACTORY_H
