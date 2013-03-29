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

class GUIIMAGELIST : public GUIWIDGETLIST
{
public:
	GUIIMAGELIST();

	~GUIIMAGELIST();

	/// Create image elements. To be called after SetupList!
	void SetupDrawable(
		SCENENODE & scene,
		ContentManager & content,
		const std::string & path,
		const std::string & ext,
		float z);

	/// Special case: list of identical images
	void SetImage(const std::string & value);

protected:
	/// verboten
	GUIIMAGELIST(const GUIIMAGELIST & other);
	GUIIMAGELIST & operator=(const GUIIMAGELIST & other);

	/// called during Update to process m_values
	void UpdateElements(SCENENODE & scene);
};

#endif // _GUIIMAGELIST_H
