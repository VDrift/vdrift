#ifndef _BCN_DECODE_H
#define _BCN_DECODE_H

// bcn = 1, 2, 3, 5, 7: 4 bytes-per-pixel
// bcn = 4, 1 byte-per-pixel
// bcn = 6, 16 bytes-per-pixel (32-bit float)
// sign = 0, bc6 data is unsigned
int BcnDecode(
	void *dst, int dst_size,
	const void *src, int src_size,
	int width, int height,
	int bcn, int sign, int yflip);

#endif //_BCN_DECODE_H
