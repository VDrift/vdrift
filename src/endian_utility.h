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
/************************************************************************/

#ifndef _ENDIAN_UTILITY_H
#define _ENDIAN_UTILITY_H

#ifdef __BIG_ENDIAN__
	#define ENDIAN_SWAP_16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | \
		(((uint16_t)(A) & 0x00ff) << 8))
	#define ENDIAN_SWAP_32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
		(((uint32_t)(A) & 0x00ff0000) >> 8)  | \
		(((uint32_t)(A) & 0x0000ff00) << 8)  | \
		(((uint32_t)(A) & 0x000000ff) << 24))
	#define ENDIAN_SWAP_64(x) (((_int64)(ntohl((int)((x << 32) >> 32))) << 32) | (unsigned int)ntohl(((int)(x >> 32))))
	#define ENDIAN_SWAP_FLOAT(A)  LoadLEFloat(&(A))
	inline float LoadLEFloat ( float *f )
	{
		#define __stwbrx( value, base, index ) \
			__asm__ ( "stwbrx %0, %1, %2" :  : "r" (value), "b%" (index), "r" (base) : "memory" )

		union
		{
			long            i;
			float           f;
		} transfer;

		//load the float into the integer unit
		unsigned int    temp = ( ( long* ) f ) [0];

		//store it to the transfer union, with byteswapping
		__stwbrx ( temp,  &transfer.i, 0 );

		//load it into the FPU and return it
		return transfer.f;
	}
#else
	#define ENDIAN_SWAP_16(A)  (A)
	#define ENDIAN_SWAP_32(A)  (A)
	#define ENDIAN_SWAP_64(A)  (A)
	#define ENDIAN_SWAP_FLOAT(A)  (A)
#endif

#endif
