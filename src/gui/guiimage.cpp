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
#include "graphics/texture.h"

GuiImage::GuiImage() :
	m_content(0),
	m_load(false)
{
	set_image.call.bind<GuiImage, &GuiImage::SetImage>(this);
}

GuiImage::~GuiImage()
{
	// dtor
}

void GuiImage::Update(SceneNode & scene, float /*dt*/)
{
	if (m_update)
	{
		Drawable & d = GetDrawable(scene);
		d.SetColor(m_rgb[0], m_rgb[1], m_rgb[2], m_alpha);
		d.SetDrawEnable(!m_name.empty() && m_visible && m_alpha > 0);
		m_update = false;

		if (m_load)
		{
			assert(m_content);
			TextureInfo texinfo;
			texinfo.mipmap = false;
			texinfo.repeatu = false;
			texinfo.repeatv = false;
			m_content->load(m_texture, m_path, m_name + m_ext, texinfo);
			d.SetTextures(m_texture->GetId());
			m_load = false;
		}
	}
}

void GuiImage::SetupDrawable(
	SceneNode & scene,
	ContentManager & content,
	const std::string & path,
	const std::string & ext,
	const float xywh[4],
	const float uv[4],
	const float z)
{
	m_content = &content;
	m_path = path;
	m_ext = ext;

	float x1 = xywh[0] - xywh[2] * 0.5f;
	float y1 = xywh[1] - xywh[3] * 0.5f;
	float x2 = xywh[0] + xywh[2] * 0.5f;
	float y2 = xywh[1] + xywh[3] * 0.5f;
	m_varray.SetTo2DQuad(x1, y1, x2, y2, uv[0], uv[1], uv[2], uv[3]);
	m_draw = scene.GetDrawList().twodim.insert(Drawable());

	Drawable & d = GetDrawable(scene);
	d.SetVertArray(&m_varray);
	d.SetCull(false);
	d.SetDrawOrder(z);
	d.SetDrawEnable(false);
}

bool GuiImage::GetProperty(const std::string & name, Slot1<const std::string &> *& slot)
{
	if (name == "image")
		return (slot = &set_image);
	return GuiWidget::GetProperty(name, slot);
}

void GuiImage::SetImage(const std::string & value)
{
	if (m_name != value)
	{
		m_name = value;
		m_load = !value.empty();
		m_update = true;
	}
}
