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

#ifndef _OPTIONAL_H
#define _OPTIONAL_H

#include <cassert>

template <typename T>
class optional
{
	private:
		T value;
		bool value_valid;

		bool is_initialized() const {return value_valid;}

	public:
		optional() : value(T()), value_valid(false) {}
		optional(const T newvalue) : value(newvalue), value_valid(true) {}

		operator bool() const {return is_initialized();}

		T get() {assert(is_initialized());return value;}
		const T get() const {assert(is_initialized());return value;}
		T get_or_default(T thedefault) {return is_initialized()?get():thedefault;}
};

#endif
