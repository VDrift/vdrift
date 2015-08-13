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

GuiImageList::GuiImageList()
{
	//ctor
}

GuiImageList::~GuiImageList()
{
	//dtor
}

void GuiImageList::SetupDrawable(
	SceneNode & scene,
	ContentManager & content,
	const std::string & path,
	const std::string & ext,
	const float uv[4],
	float z)
{
	m_elements.resize(m_rows * m_cols);
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		float x, y;
		GetElemPos(i, x, y);

		float xywh[4] = {x + m_elemw * 0.5f, y + m_elemh * 0.5f, m_elemw, m_elemh};

		GuiImage * element = new GuiImage();
		element->SetupDrawable(scene, content, path, ext, xywh, uv, z);
		m_elements[i] = element;
	}
}

void GuiImageList::SetImage(const std::string & value)
{
	for (size_t i = 0; i < m_elements.size(); ++i)
	{
		static_cast<GuiImage*>(m_elements[i])->SetImage(value);
	}
}

void GuiImageList::UpdateElements(SceneNode & /*scene*/)
{
	assert(m_values.size() <= m_elements.size());
	for (size_t i = 0; i < m_values.size(); ++i)
	{
		static_cast<GuiImage*>(m_elements[i])->SetImage(m_values[i]);
	}
	for (size_t i = m_values.size(); i < m_elements.size(); ++i)
	{
		static_cast<GuiImage*>(m_elements[i])->SetImage("");
	}
}
