#ifndef _PNG_H
#define _PNG_H

#include <vector>

// 2 GB max, 8 bit depth, no palette, no interlacing, returns 0 on success
unsigned LoadPNG(const char * filepath, std::vector<unsigned char> & img,
				unsigned & w, unsigned & h, unsigned char & ch);

// Get error string from LoadPNG return code
const char * LoadPNGError(unsigned ret);
#endif //_PNG_H

