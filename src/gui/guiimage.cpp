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

#include "guiimage.h"
#include "content/contentmanager.h"

GUIIMAGE::GUIIMAGE()
{
	set_image.call.bind<GUIIMAGE, &GUIIMAGE::SetImage>(this);
}

GUIIMAGE::~GUIIMAGE()
{
	// dtor
}

void GUIIMAGE::Update(SCENENODE & scene, float dt)
{
	if (m_update)
	{
		DRAWABLE & d = GetDrawable(scene);
		if (m_name.empty())
		{
			m_visible = false;
		}
		else
		{
			assert(m_content);
			TEXTUREINFO texinfo;
			texinfo.mipmap = false;
			texinfo.repeatu = false;
			texinfo.repeatv = false;
			std::tr1::shared_ptr<TEXTURE> texture;
			m_content->load(texture, m_path, m_name + m_ext, texinfo);
			d.SetDiffuseMap(texture);
			m_visible = true;
		}
		GUIWIDGET::Update(scene, dt);
	}
}

void GUIIMAGE::SetupDrawable(
	SCENENODE & scene,
	ContentManager & content,
	const std::string & path,
	const std::string & ext,
	float x, float y, float w, float h, float z)
{
	m_content = &content;
	m_path = path;
	m_ext = ext;
	m_varray.SetToBillboard(x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f);
	m_draw = scene.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & d = GetDrawable(scene);
	d.SetVertArray(&m_varray);
	d.SetCull(false, false);
	d.SetDrawOrder(z);
}

void GUIIMAGE::SetImage(const std::string & value)
{
	if (m_name != value)
	{
		m_name = value;
		m_update = true;
	}
}
