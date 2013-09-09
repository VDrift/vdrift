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

////////////////////////////////////////////////////////////////////////////
//
// Using Remi Coulom's K1999 Path-Optimisation Algorithm to calculate
// racing line.
//
// This is an adaption of Remi Coulom's K1999 driver for TORCS
//
////////////////////////////////////////////////////////////////////////////

#ifndef _K1999_H
#define _K1999_H

#include <vector>
#include <iosfwd>

class RoadStrip;

class K1999
{
private:
	std::vector <double> tx;
	std::vector <double> ty;
	std::vector <double> tRInverse;
	std::vector <double> txLeft;
	std::vector <double> tyLeft;
	std::vector <double> txRight;
	std::vector <double> tyRight;
	std::vector <double> tLane;
	int Divs;

	void UpdateTxTy(int i);
	double GetRInverse(int prev, double x, double y, int next);
	void AdjustRadius(int prev, int i, int next, double TargetRInverse, double Security = 0);
	void Smooth(int Step);
	void StepInterpolate(int iMin, int iMax, int Step);
	void Interpolate(int Step);

#ifdef DRAWPATH
	void DrawPath(std::ostream &out);
#endif

public:
	bool LoadData(const RoadStrip & road);
	void CalcRaceLine();
	void UpdateRoadStrip(RoadStrip & road);
};

#endif //_K1999_H
