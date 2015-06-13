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

#include "configfactory.h"
#include "contentmanager.h"
#include "cfg/ptree.h"
#include <fstream>

class ConfigInclude : public Include
{
public:
	ConfigInclude(
		ContentManager & content,
		const std::string & basepath,
		const std::string & path) :
		content(content),
		basepath(basepath),
		path(path)
	{
		// ctor
	}

	void operator()(PTree & node, std::string & value)
	{
		std::shared_ptr<PTree> sptr;
		if (content.load(sptr, path, value))
		{
			node.set(*sptr);
		}
	}

private:
	ContentManager & content;
	const std::string & basepath;
	const std::string & path;
};

Factory<PTree>::Factory() :
	m_default(new PTree()),
	m_read(&read_ini),
	m_write(&write_ini),
	m_content(0)
{
	// ctor
}

void Factory<PTree>::init(
	void (&read)(std::istream &, PTree &, Include *),
	void (&write)(const PTree &, std::ostream &),
	ContentManager & content)
{
	m_read = &read;
	m_write = &write;
	m_content = &content;
}

template <>
bool Factory<PTree>::create(
	std::shared_ptr<PTree> & sptr,
	std::ostream & /*error*/,
	const std::string & basepath,
	const std::string & path,
	const std::string & name,
	const empty&)
{
	const std::string abspath = basepath + "/" + path + "/" + name;
	std::ifstream file(abspath.c_str());
	if (file.good())
	{
		std::shared_ptr<PTree> temp(new PTree());
		if (m_content)
		{
			// include support
			ConfigInclude include(*m_content, basepath, path);
			m_read(file, *temp, &include);
		}
		else
		{
			m_read(file, *temp, 0);
		}
		sptr = temp;
		return true;
	}
	//m_error << "Failed to load " << abspath << std::endl;
	return false;
}

// replace file string with stream
template <>
bool Factory<PTree>::create(
	std::shared_ptr<PTree> & sptr,
	std::ostream & /*error*/,
	const std::string & /*basepath*/,
	const std::string & /*path*/,
	const std::string & /*name*/,
	const std::string& file)
{
	std::istringstream s(file);
	if (s.good())
	{
		std::shared_ptr<PTree> temp(new PTree());
		m_read(s, *temp, 0);
		sptr = temp;
		return true;
	}
	//m_error << "Failed to load " << abspath << std::endl;
	return false;
}

const std::shared_ptr<PTree> & Factory<PTree>::getDefault() const
{
	return m_default;
}
