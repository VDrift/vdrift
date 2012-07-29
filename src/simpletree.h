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

#ifndef _SIMPLETREE_H
#define _SIMPLETREE_H

#include <map>
#include <iostream>

template<typename KEYTYPE, typename VALUETYPE>
class SIMPLETREE
{
private:
	void DebugPrint(int level, std::ostream & mystream) const
	{
		mystream << value << endl;

		for (typename std::map <KEYTYPE,SIMPLETREE>::iterator i = branch.begin();
				   i != branch.end(); i++)
		{
			for (int n = 0; n < level; n++)
				mystream << "-";

			mystream << i->first << "=";
			i->second.DebugPrint(level+1, mystream);
		}
	}

public:
	SIMPLETREE() : parent(NULL) {}

	SIMPLETREE * parent;
	VALUETYPE value;
	std::map <KEYTYPE, SIMPLETREE> branch;

	void DebugPrint() const {DebugPrint(0,cout);}
};


#endif
