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

#ifndef _RANDOM_H
#define _RANDOM_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

class DeterministicRandom
{
public:
	DeterministicRandom();
	void ReSeed();
	void ReSeed(unsigned int newseed);

	double Get();
	double Peek() const;

private:
	unsigned int seed;
};

class Random
{
public:
	Random();
	void ReSeed();
	void ReSeed(unsigned int newseed);

	double Get();
private:
	unsigned int seed;
};

#endif
