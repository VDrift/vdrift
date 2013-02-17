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

#ifndef _GUILIST_H
#define _GUILIST_H

/// gui element list base class
class GUILIST
{
public:
	/// rows/cols the number of visible list elements
	/// xmin/max and ymin/max define list dimensions
	/// xpad and ypad the padding around list elements
	/// vetical forces vertical list element flow over horizontal
	void SetupList(
		unsigned rows, unsigned cols,
		float xmin, float ymin,
		float xmax, float ymax,
		float xpad, float ypad,
		bool vertical = true);

protected:
	unsigned m_rows, m_cols;
	float m_elemw, m_elemh;
	float m_xmin, m_ymin;
	float m_xmax, m_ymax;
	float m_xpad, m_ypad;
	bool m_vertical;

	unsigned m_active_element;
	int m_list_offset;

	/// get nth list element position
	void GetElemPos(unsigned n, float & x, float & y) const;

	/// used as base class only
	GUILIST();
	~GUILIST();
};

inline void GUILIST::SetupList(
	unsigned rows, unsigned cols,
	float xmin, float ymin,
	float xmax, float ymax,
	float xpad, float ypad,
	bool vertical)
{
	m_rows = rows; m_cols = cols;
	m_xmin = xmin; m_ymin = ymin;
	m_xmax = xmax; m_ymax = ymax;
	m_xpad = xpad; m_ypad = ypad;
	m_vertical = vertical;

	float listw = (m_xmax - m_xmin);
	float listh = (m_ymax - m_ymin);
	m_elemw = (listw - m_cols * m_xpad * 2) / m_cols;
	m_elemh = (listh - m_rows * m_ypad * 2) / m_rows;
}

inline void GUILIST::GetElemPos(unsigned n, float & x, float & y) const
{
	unsigned col, row;
	if (m_vertical)
	{
		col = n / m_rows;
		row = n - col * m_rows;
	}
	else
	{
		row = n / m_cols;
		col = n - row * m_cols;
	}
	x = m_xmin + (col + 0.5f) * (m_elemw + 2 * m_xpad);
	y = m_ymin + (row + 0.5f) * (m_elemh + 2 * m_ypad);
}

inline GUILIST::GUILIST() :
	m_rows(1), m_cols(1),
	m_xmin(0), m_ymin(0),
	m_xmax(1), m_ymax(1),
	m_xpad(0), m_ypad(0),
	m_vertical(true),
	m_active_element(0),
	m_list_offset(0)
{
	// ctor
}

inline GUILIST::~GUILIST()
{
	// dtor
}

#endif // _GUILIST_H
