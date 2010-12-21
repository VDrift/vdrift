#ifndef _UTILS_H
#define _UTILS_H

#include <iostream>
#include <string>

namespace UTILS
{

///seeks to the token and returns the text minus the token
/// if we got to the end of the file, return what we have so far
std::string SeekTo(std::istream & in, const std::string & token);

std::string LoadFileIntoString(const std::string & filepath, std::ostream & error_output);

};

#endif
