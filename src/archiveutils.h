#ifndef _ARCHIVE_H
#define _ARCHIVE_H

#include <string>
#include <iostream>

// returns true on success
bool Decompress(const std::string & file, const std::string & output_path, std::ostream & info_output, std::ostream & error_output);

#endif