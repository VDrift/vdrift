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

#ifndef _GUI_SLIDER_H
#define _GUI_SLIDER_H

#include "guiwidget.h"
#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"
#include <memory>

// gui slider base class

class Texture;

class GuiSlider : public GuiWidget
{
public:
	GuiSlider() {};

	void SetValue(const std::string & value);

	void SetMinValue(const std::string & value);

	void SetMaxValue(const std::string & value);

protected:
	std::shared_ptr<Texture> m_texture;
	SceneNode::DrawableHandle m_draw;
	VertexArray m_varray;
	float m_x = 0, m_y = 0;
	float m_w = 0, m_h = 0;
	float m_min_value = -0.02, m_max_value = 0.02;  // relative slider element extents

	GuiSlider(const GuiSlider & other);

	Drawable & GetDrawable(SceneNode & node) override;

	void InitDrawable(
		SceneNode & node,
		const std::shared_ptr<Texture> & texture,
		float xywh[4],
		float z);
};

#endif
