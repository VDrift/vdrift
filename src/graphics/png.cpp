#include "png.h"
#include "defer.h"

#include "zlib.h"
#include <cassert>
#include <cstdio>

#define IHDR 0x49484452
#define IDAT 0x49444154
#define IEND 0x49454E44
#define FNONE  0
#define FSUB   1
#define FUP    2
#define FAVG   3
#define FPAETH 4
#define TWOGB (1u << 31)
#define ABS(x) ((x) < 0 ? -(x) : (x))

inline unsigned read32be(const unsigned char* buffer) {
	return ((unsigned)buffer[0] << 24) | ((unsigned)buffer[1] << 16) |
			((unsigned)buffer[2] << 8) | (unsigned)buffer[3];
}

inline unsigned char paeth(int a, int b, int c) {
	int pa = ABS(b - c);
	int pb = ABS(a - c);
	int pc = ABS(a + b - c - c);
	if (pb < pa) { a = b; pa = pb; }
	return pc < pa ? c : a;
}

// Unfilter in row using previous pout row into out row
inline unsigned unfilter(unsigned char* out, const unsigned char* in, const unsigned char* pout,
							unsigned length, unsigned pixelwidth, unsigned char filter) {
	// Enforce pixelwidth 1-4, mostly to help the compiler optimizing the loops
	if (pixelwidth < 1) pixelwidth = 1;
	if (pixelwidth > 4) pixelwidth = 4;
	if (pixelwidth > length) {
		return 1; // invalid length
	}
	unsigned i, j;
	switch (filter) {
	case FNONE:
		for (i = 0; i != length; ++i) {
			out[i] = in[i];
		}
		break;
	case FSUB: {
		for (i = 0; i != pixelwidth; ++i) {
			out[i] = in[i];
		}
		for (j = 0; i != length; ++i, ++j) {
			out[i] = in[i] + out[j];
		}
		break;
	}
	case FUP:
		if (pout) {
			for (i = 0; i != length; ++i) {
				out[i] = in[i] + pout[i];
			}
		} else {
			for (i = 0; i != length; ++i) {
				out[i] = in[i];
			}
		}
		break;
	case FAVG:
		if (pout) {
			for (i = 0; i != pixelwidth; ++i) {
				out[i] = in[i] + (pout[i] >> 1u);
			}
			for (j = 0; i != length; ++i, ++j) {
				out[i] = in[i] + ((out[j] + pout[i]) >> 1u);
			}
		} else {
			for (i = 0; i != pixelwidth; ++i) {
				out[i] = in[i];
			}
			for (j = 0; i != length; ++i, ++j) {
				out[i] = in[i] + (out[j] >> 1u);
			}
		}
		break;
	case FPAETH:
		if (pout) {
			for (i = 0; i != pixelwidth; ++i) {
				out[i] = in[i] + pout[i]; // paeth(0, pout[i], 0) is pout[i]
			}
			for (j = 0; i != length; ++i, ++j) {
				out[i] = in[i] + paeth(out[j], pout[i], pout[j]);
			}
		} else {
			for (i = 0; i != pixelwidth; ++i) {
				out[i] = in[i];
			}
			for (j = 0; i != length; ++i, ++j) {
				out[i] = in[i] + out[j]; // paeth(out[j], 0, 0) is out[j]
			}
		}
		break;
	default:
		return 2; // invalid filter type
	}
	return 0;
}

unsigned LoadPNG(const char * filepath, std::vector<unsigned char> & img,
			unsigned & w, unsigned & h, unsigned char & ch)
{
	FILE * file = fopen(filepath, "rb");
	if (!file) {
		return 1;
	}
	defer {
		fclose(file);
	};

	// Signature, chunk length, type, header chunk data, crc
	unsigned char hd[8 + 4 + 4 + 13 + 4];
	if (fread(hd, 1, sizeof(hd), file) != sizeof(hd)) {
		return 1;
	}

	// Check PNG signature
	if (hd[0] != 137 || hd[1] != 80 || hd[2] != 78 || hd[3] != 71 ||
		hd[4] !=  13 || hd[5] != 10 || hd[6] != 26 || hd[7] != 10) {
		return 2;
	}

	// IHDR chunk comes first and is 13 bytes long
	unsigned length = read32be(hd + 8);
	unsigned type = read32be(hd + 12);
	if (type != IHDR) {
		return 3;
	}
	if (length != 13) {
		return 4;
	}

	// IHDR data
	const unsigned x = read32be(hd + 16);
	const unsigned y = read32be(hd + 20);
	const unsigned depth = hd[24];
	const unsigned color = hd[25];
	const unsigned comp = hd[26];
	const unsigned filter = hd[27];
	const unsigned interlace = hd[28];

	// 8 bit depth, no palette, default compression, default filter, no interlacing
	if (x == 0 || y == 0 || comp != 0 || filter != 0) {
		return 5;
	}
	if (depth != 8) {
		return 6;
	}
	if (color == 3) {
		return 7;
	}
	if (interlace != 0) {
		return 8;
	}

	const unsigned char channels = 1 + (color & 2) + ((color & 4) >> 2);// - ((color & 1) << 1); palette
	const unsigned image_width = channels * x;

	// 2 GB limit
	if (y > TWOGB / image_width) {
		return 9;
	}

	// Use the same buffer for decoding, filtering and the final image
	const unsigned image_size = image_width * y;
	const unsigned raw_length = image_size + y;
	const unsigned buffer_length = raw_length + image_width;
	img.resize(buffer_length);

	// Decoding buffer
	unsigned char* raw = img.data() + image_width;

	// Init decoder
	const unsigned zbuffer_length = 8 * 1024;
	unsigned char zbuffer[zbuffer_length];
	z_stream zs;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;
	zs.avail_out = raw_length;
	zs.next_out = raw;
	if (inflateInit(&zs) != Z_OK) {
		return 10;
	}
	defer {
		inflateEnd(&zs);
	};

	// Read and decode IDAT chunks
	while (!feof(file)) {
		unsigned char in[8];
		if (fread(in, 1, 8, file) != 8) {
			return 1;
		};
		length = read32be(in);
		type = read32be(in + 4);
		if (type == IDAT) {
			// Max idat length check
			if (length >= TWOGB) {
				return 11;
			}
			// Read and decode
			while (length) {
				unsigned avail_in = zbuffer_length;
				if (avail_in > length)
					avail_in = length;
				zs.avail_in = avail_in;
				zs.next_in = zbuffer;
				if (fread(zbuffer, 1, avail_in, file) != avail_in) {
					return 1;
				};
				int ret = inflate(&zs, Z_NO_FLUSH);
				assert(ret != Z_STREAM_ERROR);
				assert(ret != Z_BUF_ERROR);
				switch (ret) {
					case Z_STREAM_END:
						if (zs.total_out != raw_length) {
							return 12;
						}
						break;
					case Z_NEED_DICT:
						return 13;
					case Z_DATA_ERROR:
						return 14;
					case Z_MEM_ERROR:
						return 15;
				}
				length -= avail_in;
			}
			// skip crc
			if (fread(in, 1, 4, file) != 4) {
				return 1;
			};
		}
		else if (type == IEND) {
			break;
		}
		else {
			// Skip other chunks + crc
			if (fseek(file, length + 4, SEEK_CUR) != 0) {
				return 1;
			};
		}
	}

	// Apply filters
	unsigned char* out = &img[0];
	const unsigned char* in = raw;
	const unsigned char* pout = 0;
	for (unsigned row = 0; row < y; ++row) {
		unsigned char row_filter = *in++;
		if (unfilter(out, in, pout, image_width, channels, row_filter)) {
			return 16;
		}
		pout = out;
		in += image_width;
		out += image_width;
	}

	// drop buffer bytes
	img.resize(image_size);
	w = x;
	h = y;
	ch = channels;
	return 0;
}

const char * LoadPNGError(unsigned ret) {
	static const char* s[] = {
		"Unknown error",
		"File read error",
		"Not a PNG file",
		"First chunk is not IHDR",
		"Wrong IHDR length",
		"Malformed IHDR data",
		"Depth not 8 bit",
		"Indexed color not supported",
		"Interlacing not supported",
		"Image larger than 2 GB",
		"Inflate init failure",
		"IDAT too large",
		"Premature inflate end",
		"Inflate dict error",
		"Inflate data error",
		"Inflate memory error",
		"Image filter error",
	};
	if (ret > sizeof(s) / sizeof(s[0])) {
		ret = 0;
	}
	return s[ret];
}
