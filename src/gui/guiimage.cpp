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

void GuiImage::Update(SceneNode & scene, float dt)
{
	GuiWidget::Update(scene, dt);
	Drawable & d = GetDrawable(scene);
	if (d.GetDrawEnable() && m_load)
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

void GuiImage::SetupDrawable(
	SceneNode & scene,
	ContentManager & content,
	const std::string & path,
	const std::string & ext,
	float x, float y, float w, float h, float z)
{
	m_content = &content;
	m_path = path;
	m_ext = ext;
	m_varray.SetToBillboard(x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f);
	m_draw = scene.GetDrawList().twodim.insert(Drawable());
	m_visible = false;
	Drawable & d = GetDrawable(scene);
	d.SetVertArray(&m_varray);
	d.SetCull(false);
	d.SetDrawOrder(z);
}

void GuiImage::SetImage(const std::string & value)
{
	if (m_name != value)
	{
		m_name = value;
		m_load = !value.empty();
		m_visible = m_load;
		m_update = true;
	}
}
