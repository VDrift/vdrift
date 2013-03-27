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

#include "guiimagelist.h"
#include "guiimage.h"

GUIIMAGELIST::GUIIMAGELIST()
{
	//ctor
}

GUIIMAGELIST::~GUIIMAGELIST()
{
	//dtor
}

void GUIIMAGELIST::SetupDrawable(
	SCENENODE & scene, ContentManager & content,
	const std::string & path, float z)
{
	m_elements.resize(m_rows * m_cols);
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		float x, y;
		GetElemPos(i, x, y);

		GUIIMAGE * element = new GUIIMAGE();
		element->SetupDrawable(
			scene, content, path,
			x, y, m_elemw, m_elemh, z);

		m_elements[i] = element;
	}
}

void GUIIMAGELIST::SetImage(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		static_cast<GUIIMAGE*>(m_elements[i])->SetImage(value);
	}
}

void GUIIMAGELIST::UpdateElements(SCENENODE & scene)
{
	assert(m_values.size() <= m_elements.size());
	for (size_t i = 0; i < m_values.size(); ++i)
	{
		static_cast<GUIIMAGE*>(m_elements[i])->SetImage(m_values[i]);
	}
	for (size_t i = m_values.size(); i < m_elements.size(); ++i)
	{
		static_cast<GUIIMAGE*>(m_elements[i])->SetImage("");
	}
}
