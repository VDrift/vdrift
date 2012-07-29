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

#ifndef _MACROS_H
#define _MACROS_H

#define _SERIALIZE_(ser,varname) if (!ser.Serialize(#varname,varname)) return false
#define _SERIALIZEENUM_(ser,varname,type) if (ser.GetIODirection() == joeserialize::Serializer::DIRECTION_INPUT) {int _enumint(0);if (!ser.Serialize(#varname,_enumint)) return false;varname=(type)_enumint;} else {int _enumint = varname;if (!ser.Serialize(#varname,_enumint)) return false;}

///break up the input into a vector of strings using the token characters given
std::vector <std::string> Tokenize(const std::string & input, const std::string & tokens);

#endif
