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

#ifndef _GUI_LINEAR_SLIDER_H
#define _GUI_LINEAR_SLIDER_H

#include "guislider.h"

class GuiLinearSlider : public GuiSlider
{
public:
	GuiLinearSlider();

	~GuiLinearSlider();

	virtual void Update(SceneNode & scene, float dt);

	void SetupDrawable(
		SceneNode & node,
		const std::shared_ptr<Texture> & texture,
		float xywh[4], float z, bool vertical,
  		std::ostream & error_output);

private:
	bool m_vertical;

	GuiLinearSlider(const GuiLinearSlider & other);

	void UpdateVertexArray();
};

#endif // _GUI_LINEAR_SLIDER_H
