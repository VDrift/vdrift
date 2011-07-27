#include "archiveutils.h"

#include "pathmanager.h"

#include <archive.h>
#include <archive_entry.h>

#include <fstream>

bool Decompress(const std::string & file, const std::string & output_path, std::ostream & info_output, std::ostream & error_output)
{
	const bool verbose = true;
	
	// ensure the output path exists
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
				info_output << "Creating      " << filename << std::endl;
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
				{
					f.write(buff, size);
				}
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
