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

#include "guilabellist.h"
#include "guilabel.h"

GUILABELLIST::GUILABELLIST() :
	m_scalex(0.1), m_scaley(0.1), m_z(0), m_font(0), m_align(0)
{
	// ctor
}

GUILABELLIST::~GUILABELLIST()
{
	// dtor
}

void GUILABELLIST::SetupDrawable(
	const FONT & font, int align,
	float scalex, float scaley, float z)
{
	m_scalex = scalex;
	m_scaley = scaley;
	m_z = z;
	m_font = &font;
	m_align = align;
}

void GUILABELLIST::UpdateElements(SCENENODE & scene)
{
	unsigned num_new = m_values.size();
	unsigned num_old = m_elements.size();
	for (unsigned i = num_new; i < num_old; ++i)
	{
		static_cast<GUILABEL*>(m_elements[i])->Remove(scene);
		delete m_elements[i];
	}
	m_elements.resize(num_new);
	for (unsigned i = num_old; i < num_new; ++i)
	{
		float x, y;
		GetElemPos(i, x, y);

		GUILABEL * element = new GUILABEL();
		element->SetupDrawable(
			scene, *m_font, m_align, m_scalex, m_scaley,
			x, y, m_elemw, m_elemh, m_z);

		m_elements[i] = element;
	}
	for (unsigned i = 0; i < num_new; ++i)
	{
		static_cast<GUILABEL*>(m_elements[i])->SetText(m_values[i]);
	}
}

DRAWABLE & GUILABELLIST::GetDrawable(SCENENODE & scene)
{
	assert(0); // don't want to end up here
	return m_elements[0]->GetDrawable(scene);
}
