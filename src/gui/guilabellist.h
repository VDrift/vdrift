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

class FONT;

class GUILABELLIST : public GUIWIDGETLIST
{
public:
	GUILABELLIST();

	~GUILABELLIST();

	void SetupDrawable(
		const FONT & font, int align,
		float scalex, float scaley, float z);

protected:
	float m_scalex, m_scaley, m_z;
	const FONT * m_font;
	int m_align;

	/// verboten
	GUILABELLIST(const GUILABELLIST & other);

	/// called during Update to process m_values
	void UpdateElements(SCENENODE & scene);

	/// ugh, dead weight
	DRAWABLE & GetDrawable(SCENENODE & scene);
};

#endif // _GUILABELLIST_H
