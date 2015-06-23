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

#ifndef _CONFIGFACTORY_H
#define _CONFIGFACTORY_H

#include "contentfactory.h"

class PTree;
class ContentManager;
struct Include;

template <>
class Factory<PTree>
{
public:
	struct empty {};

	Factory();

	// content manager is needed for include functionality
	void init(
		void (&read)(std::istream &, PTree &, Include *),
		void (&write)(const PTree &, std::ostream &),
		ContentManager & content);

	template <class P>
	bool create(
		std::shared_ptr<PTree> & sptr,
		std::ostream & error,
		const std::string & basepath,
		const std::string & path,
		const std::string & name,
		const P & param);

	const std::shared_ptr<PTree> & getDefault() const;

private:
	std::shared_ptr<PTree> m_default;
	void (*m_read)(std::istream &, PTree &, Include *);
	void (*m_write)(const PTree &, std::ostream &);
	ContentManager * m_content;
};

#endif // _CONFIGFACTORY_H
