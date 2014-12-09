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

#ifndef _GUIWIDGET_H
#define _GUIWIDGET_H

#include "signalslot.h"
#include <string>

class SceneNode;
class Drawable;

/// Widget base class
class GuiWidget
{
public:
	/// base destructor
	virtual ~GuiWidget() {};

	/// update widget state
	virtual void Update(SceneNode & scene, float dt);

	/// scale widget alpha (opacity) [0, 1]
	virtual void SetAlpha(SceneNode & scene, float value);

	/// todo: need to ge rid of this one
	virtual Drawable & GetDrawable(SceneNode & scene) = 0;

	/// properties
	virtual bool GetProperty(const std::string & name, Slot1<const std::string &> *& slot);

	void SetHSV(float h, float s, float v);
	void SetRGB(float r, float g, float b);
	void SetOpacity(float value);
	void SetHue(float value);
	void SetSat(float value);
	void SetVal(float value);

	void SetVisible(const std::string & value);
	void SetOpacity(const std::string & value);
	void SetColor(const std::string & value);
	void SetHue(const std::string & value);
	void SetSat(const std::string & value);
	void SetVal(const std::string & value);

	Slot1<const std::string &> set_visible;
	Slot1<const std::string &> set_opacity;
	Slot1<const std::string &> set_color;
	Slot1<const std::string &> set_hue;
	Slot1<const std::string &> set_sat;
	Slot1<const std::string &> set_val;

protected:
	float m_rgb[3];
	float m_hsv[3];
	float m_alpha;
	bool m_visible;
	bool m_update;

	GuiWidget();
};

#endif // _GUIWIDGET_H
