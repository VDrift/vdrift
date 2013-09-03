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
template<typename Base>
class Derived
{
	private:
		Base * ptr;

	public:
		Derived() : ptr(0) {}
		Derived(Base * newobj) {ptr = newobj;}
		Derived(const Derived & other) : ptr(0) {operator=(other);}
		Derived & operator= (Base * newobj) {if (ptr) delete ptr;ptr=newobj;return *this;}
		~Derived() {if (ptr) delete ptr;}
		Base * Get() {return ptr;}
		const Base * Get() const {return ptr;}
		const Base * GetReadOnly() const {return ptr;}
		Base * operator-> () {assert(ptr);return ptr;}
		const Base * operator-> () const {assert(ptr);return ptr;}
		Base & operator* ()  { assert(ptr);return *ptr; }
		const Base & operator* () const { assert(ptr);return *ptr; }
		Derived & operator= (const Derived & other)
		{
			if (other.GetReadOnly())
				ptr = other.GetReadOnly()->clone();
			return *this;
		}
		Derived & operator= (const Base & other)
		{
			ptr = other.clone();
			return *this;
		}
};

#endif
