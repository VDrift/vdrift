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

#ifndef _DERIVED_H
#define _DERIVED_H

#include <memory>
#include <cassert>

///a wrapper class to make it easy to put derived classes into STL containers
template<typename T_BASECLASS>
class DERIVED
{
	private:
		T_BASECLASS * ptr;

	public:
		DERIVED() : ptr(0) {}
		DERIVED(T_BASECLASS * newobj) {ptr = newobj;}
		DERIVED(const DERIVED & other) : ptr(0) {operator=(other);}
		DERIVED & operator= (T_BASECLASS * newobj) {if (ptr) delete ptr;ptr=newobj;return *this;}
		~DERIVED() {if (ptr) delete ptr;}
		T_BASECLASS * Get() {return ptr;}
		const T_BASECLASS * Get() const {return ptr;}
		const T_BASECLASS * GetReadOnly() const {return ptr;}
		T_BASECLASS * operator-> () {assert(ptr);return ptr;}
		const T_BASECLASS * operator-> () const {assert(ptr);return ptr;}
		T_BASECLASS & operator* ()  { assert(ptr);return *ptr; }
		const T_BASECLASS & operator* () const { assert(ptr);return *ptr; }
		DERIVED & operator= (const DERIVED & other)
		{
			if (other.GetReadOnly())
				ptr = other.GetReadOnly()->clone();
			return *this;
		}
		DERIVED & operator= (const T_BASECLASS & other)
		{
			ptr = other.clone();
			return *this;
		}
};

#endif
