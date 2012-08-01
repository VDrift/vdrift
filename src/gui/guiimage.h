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

class TEXTURE;
class ContentManager;

class GUIIMAGE : public GUIWIDGET
{
public:
	GUIIMAGE();

	~GUIIMAGE();

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		ContentManager & content,
		const std::string & imagepath,
		float x, float y, float w, float h, float z);

	Slot1<const std::string &> set_image;

private:
	ContentManager * m_content;
	std::string m_imagepath, m_imagename;
	keyed_container <DRAWABLE>::handle m_draw;
	VERTEXARRAY m_varray;

	void SetImage(const std::string & value);

	DRAWABLE & GetDrawable(SCENENODE & scene)
	{
		return scene.GetDrawlist().twodim.get(m_draw);
	}

	GUIIMAGE(const GUIIMAGE & other);
};

#endif
