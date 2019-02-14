#ifndef _TOKENIZE_H
#define _TOKENIZE_H

#include <string>
#include <vector>

/// break up the input into a vector of strings using the token characters given
inline std::vector <std::string> Tokenize(const std::string & input, const std::string & tokens)
{
	std::vector <std::string> out;

	unsigned int pos = 0;
	unsigned int lastpos = 0;

	while (pos != (unsigned int) std::string::npos)
	{
		pos = input.find_first_of(tokens, pos);
		std::string thisstr = input.substr(lastpos,pos-lastpos);
		if (!thisstr.empty())
			out.push_back(thisstr);
		pos = input.find_first_not_of(tokens, pos);
		lastpos = pos;
	}

	return out;
}

#endif
