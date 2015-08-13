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

#ifndef _GUIIMAGELIST_H
#define _GUIIMAGELIST_H

#include "guiwidgetlist.h"

class ContentManager;

class GuiImageList : public GuiWidgetList
{
public:
	GuiImageList();

	~GuiImageList();

	/// Create image elements. To be called after SetupList!
	void SetupDrawable(
		SceneNode & scene,
		ContentManager & content,
		const std::string & path,
		const std::string & ext,
		const float uv[4],
		float z);

	/// Special case: list of identical images
	void SetImage(const std::string & value);

protected:
	/// verboten
	GuiImageList(const GuiImageList & other);
	GuiImageList & operator=(const GuiImageList & other);

	/// called during Update to process m_values
	void UpdateElements(SceneNode & scene);
};

#endif // _GUIIMAGELIST_H
