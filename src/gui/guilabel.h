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
	GuiLabel();

	virtual ~GuiLabel();

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SceneNode & scene,
		const Font & font, int align,
		float scalex, float scaley,
		float xywh[4], float z);

	bool GetProperty(const std::string & name, Slot1<const std::string &> *& slot);

	void SetText(const std::string & text);

	Slot1<const std::string &> set_value;

private:
	SceneNode::DrawableHandle m_draw;
	TextDraw m_text_draw;
	std::string m_text;
	const Font * m_font;
	float m_x, m_y, m_w, m_h;
	float m_scalex, m_scaley;
	int m_align;

	Drawable & GetDrawable(SceneNode & scene);
	GuiLabel(const GuiLabel & other);
};

#endif
