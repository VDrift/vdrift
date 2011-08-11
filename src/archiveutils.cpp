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

#include <archive.h>
#include <archive_entry.h>
#include <fstream>
#include "archiveutils.h"
#include "pathmanager.h"

bool Decompress(const std::string & file, const std::string & output_path, std::ostream & info_output, std::ostream & error_output)
{
	//TODO: Maybe use preprocessor definition for debug?
	const bool verbose = true;

	// Ensure the output path exists.
	PATHMANAGER::MakeDir(output_path);

	struct archive * a = archive_read_new();
	archive_read_support_compression_all(a);
	archive_read_support_format_all(a);
	int r = archive_read_open_filename(a, file.c_str(), 10240);
	if (r != ARCHIVE_OK)
	{
		error_output << "Unable to open " << file << std::endl;
		return false;
	}

	const int buffsize = 10240;
	char buff[buffsize];
	struct archive_entry *entry;
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
	{
		std::string filename = archive_entry_pathname(entry);
		std::string fullpath = output_path + "/" + filename;

		bool isdir = (archive_entry_filetype(entry) == AE_IFDIR);

		if (verbose)
		{
			if (isdir)
				info_output << "Creating " << filename << std::endl;
			else
				info_output << "Decompressing " << filename << std::endl;
		}

		if (isdir)
			PATHMANAGER::MakeDir(fullpath);
		else
		{
			std::fstream f(fullpath.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
			if (!f)
			{
				error_output << "Unable to open file for write (permissions issue?): " << fullpath << std::endl;
				return false;
			}

			int size = -1;
			while (size != 0)
			{
				size = archive_read_data(a, buff, buffsize);
				if (size < 0)
				{
					error_output << "Encountered corrupt file: " << filename << std::endl;
					return false;
				}
				else if (size != 0)
					f.write(buff, size);
			}
		}

		archive_read_data_skip(a);
	}
	r = archive_read_finish(a);
	if (r != ARCHIVE_OK)
	{
		error_output << "Unable to finish read of " << file << std::endl;
		return false;
	}

	info_output << "Successfully decompressed " << file << std::endl;
	return true;
}
