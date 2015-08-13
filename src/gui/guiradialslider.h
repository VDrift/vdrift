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

#ifndef _GUI_RADIAL_SLIDER_H
#define _GUI_RADIAL_SLIDER_H

#include "guiwidget.h"
#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"

#include <memory>

class Texture;

class GuiRadialSlider : public GuiWidget
{
public:
	GuiRadialSlider();

	~GuiRadialSlider();

	void Update(SceneNode & scene, float dt);

	void SetupDrawable(
		SceneNode & node,
		const std::shared_ptr<Texture> & texture,
		float xywh[4], float z,
		float start_angle, float end_angle, float radius,
		float dar, int fill, std::ostream & error_output);

	Slot1<const std::string &> set_value;

private:
	std::shared_ptr<Texture> m_texture;
	SceneNode::DrawableHandle m_draw;
	VertexArray m_varray;
	float m_x, m_y, m_w, m_h;
	float m_start_angle;
	float m_end_angle;
	float m_radius;
	float m_value;
	float m_dar;
	int m_fill;

	GuiRadialSlider(const GuiRadialSlider & other);

	void SetValue(const std::string & value);

	Drawable & GetDrawable(SceneNode & node);

	void InitDrawable(SceneNode & node, float draworder);

	void UpdateVertexArray();
};

#endif // _GUI_RADIAL_SLIDER_H
