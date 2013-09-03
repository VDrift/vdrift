#ifndef _DDS_H
#define _DDS_H

int IsDDS(const void *_ptr, const unsigned long _len);

int ReadDDS(
	const void *_ptr, const unsigned long _len,
	const void *&_tex, unsigned long &_texlen,
	unsigned int &_glfmt, unsigned int &_w,
	unsigned int &_h, unsigned int &_miplevels);

#endif //_DDS_H
