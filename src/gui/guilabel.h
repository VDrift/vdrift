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
#include "scenenode.h"
#include "text_draw.h"

class TEXTURE;
class FONT;

class GUILABEL : public GUIWIDGET
{
public:
	GUILABEL();

	virtual ~GUILABEL();

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		int align,
		float scalex, float scaley,
		float centerx, float centery,
		float w, float h, float z);

	void SetText(const std::string & text);

	const std::string & GetText() const;

	Slot1<const std::string &> set_value;

private:
	keyed_container<DRAWABLE>::handle m_draw;
	TEXT_DRAW m_text_draw;
	std::string m_text;
	const FONT * m_font;
	float m_x, m_y, m_w, m_h;
	float m_scalex, m_scaley;
	int m_align;

	DRAWABLE & GetDrawable(SCENENODE & scene);
	GUILABEL(const GUILABEL & other);
};

#endif
