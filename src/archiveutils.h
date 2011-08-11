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
/* This is the main entry point for VDrift.                             */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* archieveutils.cpp                                                    */
/*                                                                      */
/*   This file is responsible of decompressing repository downloaded    */
/* files like car updates or similar.                                   */
/*                                                                      */
/************************************************************************/

#ifndef _ARCHIVE_H
#define _ARCHIVE_H

#include <string>
#include <iostream>

// Returns true on success.
bool Decompress(const std::string & file, const std::string & output_path, std::ostream & info_output, std::ostream & error_output);

#endif
