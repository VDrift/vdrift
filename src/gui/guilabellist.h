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

#ifndef _GUILABELLIST_H
#define _GUILABELLIST_H

#include "guiwidgetlist.h"

class Font;

class GuiLabelList : public GuiWidgetList
{
public:
	GuiLabelList();

	~GuiLabelList();

	/// Create label elements. To be called after SetupList!
	void SetupDrawable(
		SceneNode & scene, const Font & font, int align,
		float scalex, float scaley, float z);

protected:
	/// verboten
	GuiLabelList(const GuiLabelList & other);

	/// called during Update to process m_values
	void UpdateElements(SceneNode & scene);
};

#endif // _GUILABELLIST_H
