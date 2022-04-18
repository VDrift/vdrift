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

#include "guislider.h"

class GuiRadialSlider : public GuiSlider
{
public:
	GuiRadialSlider();

	~GuiRadialSlider();

	void Update(SceneNode & scene, float dt) override;

	void SetupDrawable(
		SceneNode & node,
		const std::shared_ptr<Texture> & texture,
		float xywh[4], float z,
		float start_angle, float end_angle,
		float radius, float dar, bool pointer,
		std::ostream & error_output);

private:
	float m_start_angle;
	float m_end_angle;
	float m_radius;
	float m_dar;
	bool m_pointer;

	GuiRadialSlider(const GuiRadialSlider & other);

	void UpdateVertexArray();
};

#endif // _GUI_RADIAL_SLIDER_H
