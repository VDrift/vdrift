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

#include "contentfactory.h"
#include "sound/soundinfo.h"

class SoundBuffer;

template <>
class Factory<SoundBuffer>
{
public:
	struct empty {};

	Factory();

	/// sound device setting
	void init(const SoundInfo& value);

	template <class P>
	bool create(
		std::shared_ptr<SoundBuffer> & sptr,
		std::ostream & error,
		const std::string & basepath,
		const std::string & path,
		const std::string & name,
		const P & param);

	const std::shared_ptr<SoundBuffer> & getDefault() const;

private:
	std::shared_ptr<SoundBuffer> m_default;
	SoundInfo m_info;
};

#endif // _SOUNDFACTORY_H
