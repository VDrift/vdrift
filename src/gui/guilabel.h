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

#ifndef _GUILABEL_H
#define _GUILABEL_H

#include "guiwidget.h"
#include "text_draw.h"

class Texture;
class Font;

class GuiLabel : public GuiWidget
{
public:
	GuiLabel() {};

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SceneNode & scene,
		const Font & font, int align,
		float scalex, float scaley,
		float xywh[4], float z);

	bool GetProperty(const std::string & name, Delegated<const std::string &> & slot) override;

	void SetText(const std::string & text);

private:
	SceneNode::DrawableHandle m_draw;
	TextDraw m_text_draw;
	std::string m_text;
	const Font * m_font = 0;
	float m_x = 0, m_y = 0;
	float m_w = 0, m_h = 0;
	float m_scalex = 0, m_scaley = 0;
	int m_align = 0;

	Drawable & GetDrawable(SceneNode & scene) override;
	GuiLabel(const GuiLabel & other);
};

#endif
