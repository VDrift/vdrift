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

class SCENENODE;
class DRAWABLE;

/// Widget base class
class GUIWIDGET
{
public:
	/// base destructor
	virtual ~GUIWIDGET() {};

	/// update widget state
	virtual void Update(SCENENODE & scene, float dt);

	/// scale widget alpha [0, 1]
	virtual void SetAlpha(SCENENODE & scene, float value);

	/// override visibility
	virtual void SetVisible(SCENENODE & scene, bool value);

	/// todo: need to ge rid of this one
	virtual DRAWABLE & GetDrawable(SCENENODE & scene) = 0;

	/// properties
	void SetColor(float r, float g, float b);
	void SetOpacity(float value);
	void SetHue(float value);
	void SetSat(float value);
	void SetVal(float value);

	void SetColor(const std::string & value);
	void SetOpacity(const std::string & value);
	void SetHue(const std::string & value);
	void SetSat(const std::string & value);
	void SetVal(const std::string & value);

	/// todo: add support for typed signals
	Slot1<const std::string &> set_color;
	Slot1<const std::string &> set_opacity;
	Slot1<const std::string &> set_hue;
	Slot1<const std::string &> set_sat;
	Slot1<const std::string &> set_val;

protected:
	float m_r, m_g, m_b, m_a;	// color and alpha
	float m_h, m_s, m_v;		// hue, sat, val cache
	bool m_visible;
	bool m_update;

	GUIWIDGET();
};

#endif // _GUIWIDGET_H
