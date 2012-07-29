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

#ifndef _RESEATABLE_REFERENCE_H
#define _RESEATABLE_REFERENCE_H

#include <cassert>
#include <cstdlib>

template <typename T>
class reseatable_reference
{
	private:
		T * ptr;

		bool valid() const {return ptr;}

	public:
		reseatable_reference() : ptr(NULL) {}
		reseatable_reference(T & ref) : ptr(&ref) {}

		//default copy and assignment are OK

		operator bool() const {return valid();}

		T & get() {assert(valid());return *ptr;}
		const T & get() const {assert(valid());return *ptr;}
		T & operator*() {return get();}
		const T & operator*() const {return get();}
		T * operator->() {assert(valid());return ptr;}
		const T * operator->() const {assert(valid());return ptr;}

		void set(T & ref) {ptr = &ref;}
		reseatable_reference <T> & operator=(T & other)
		{
			set(other);
			return *this;
		}

		reseatable_reference <T> & operator=(T * other)
		{
			assert(other);
			set(*other);
			return *this;
		}

		void clear() {ptr = NULL;}
};

#endif
