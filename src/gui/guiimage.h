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

#ifndef _GUIIMAGE_H
#define _GUIIMAGE_H

#include "guiwidget.h"
#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"

#include <memory>

class ContentManager;
class Texture;

class GuiImage : public GuiWidget
{
public:
	GuiImage() {};

	void Update(SceneNode & scene, float dt) override;

	void SetupDrawable(
		SceneNode & scene,
		ContentManager & content,
		const std::string & path,
		const std::string & ext,
		const float xywh[4],
		const float uv[4],
		const float z);

	bool GetProperty(const std::string & name, Delegated<const std::string &> & slot) override;

	void SetImage(const std::string & value);

private:
	ContentManager * m_content = 0;
	std::string m_path, m_name, m_ext;
	SceneNode::DrawableHandle m_draw;
	std::shared_ptr<Texture> m_texture;
	VertexArray m_varray;
	bool m_load = false;

	Drawable & GetDrawable(SceneNode & scene) override
	{
		return scene.GetDrawList().twodim.get(m_draw);
	}

	GuiImage(const GuiImage & other);
};

#endif
