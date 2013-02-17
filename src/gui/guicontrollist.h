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

#ifndef _GUICONTROLLIST_H
#define _GUICONTROLLIST_H

#include "guilist.h"
#include "guicontrol.h"

/// a widget that mimics a list of controls
class GUICONTROLLIST : public GUICONTROL, public GUILIST
{
public:
	GUICONTROLLIST();

	~GUICONTROLLIST();

	/// Signal slots attached to selectx, selecty
	void Select(float x, float y) const;

	/// Signal slots attached to events
	void Signal(EVENT ev) const;

	/// todo: register actions

private:
	Signal1<unsigned> m_signaln[EVENTNUM];
};

#endif // _GUICONTROLLIST_H
