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
class GuiList
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
		float xpad0, float ypad0,
		float xpad1, float ypad1,
		bool vertical = true);

protected:
	unsigned m_rows, m_cols;
	float m_elemw, m_elemh;
	float m_xmin, m_ymin;
	float m_xmax, m_ymax;
	float m_xpad0, m_ypad0;
	float m_xpad1, m_ypad1;
	bool m_vertical;

	int m_list_offset;
	int m_list_size;

	/// get nth element row, column
	void GetElemPos(int n, unsigned & row, unsigned & col) const;

	/// get nth element upper left corner coordinates
	void GetElemPos(int n, float & x, float & y) const;

	/// get element index from position
	int GetElemId(float x, float y) const;

	/// used as base class only
	GuiList();
	~GuiList();
};

inline void GuiList::SetupList(
	unsigned rows, unsigned cols,
	float xmin, float ymin,
	float xmax, float ymax,
	float xpad0, float ypad0,
	float xpad1, float ypad1,
	bool vertical)
{
	m_rows = rows; m_cols = cols;
	m_xmin = xmin; m_ymin = ymin;
	m_xmax = xmax; m_ymax = ymax;
	m_xpad0 = xpad0; m_ypad0 = ypad0;
	m_xpad1 = xpad1; m_ypad1 = ypad1;
	m_vertical = vertical;

	float listw = (m_xmax - m_xmin);
	float listh = (m_ymax - m_ymin);
	float padw = (m_xpad0 + m_xpad1);
	float padh = (m_ypad0 + m_ypad1);
	m_elemw = (listw - m_cols * padw) / m_cols;
	m_elemh = (listh - m_rows * padh) / m_rows;
}

inline void GuiList::GetElemPos(int n, unsigned & col, unsigned & row) const
{
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
}

inline void GuiList::GetElemPos(int n, float & x, float & y) const
{
	unsigned col, row;
	GetElemPos(n, col, row);
	x = m_xmin + col * (m_elemw + m_xpad0 + m_xpad1) + m_xpad0;
	y = m_ymin + row * (m_elemh + m_ypad0 + m_ypad1) + m_ypad0;
}

template <typename T>
inline int clamp(T v, T vmin, T vmax)
{
	return (v < vmin) ? vmin : ((v > vmax) ? vmax : v);
}

inline int GuiList::GetElemId(float x, float y) const
{
	int col = m_cols * (x - m_xmin) / (m_xmax - m_xmin);
	int row = m_rows * (y - m_ymin) / (m_ymax - m_ymin);
	col = clamp<int>(col, 0, m_cols - 1);
	row = clamp<int>(row, 0, m_rows - 1);
	if (m_vertical)
	{
		return col * m_rows + row;
	}
	else
	{
		return row * m_cols + col;
	}
}

inline GuiList::GuiList() :
	m_rows(1), m_cols(1),
	m_xmin(0), m_ymin(0),
	m_xmax(1), m_ymax(1),
	m_xpad0(0), m_ypad0(0),
	m_xpad1(0), m_ypad1(0),
	m_vertical(true),
	m_list_offset(0),
	m_list_size(0)
{
	// ctor
}

inline GuiList::~GuiList()
{
	// dtor
}

#endif // _GUILIST_H
