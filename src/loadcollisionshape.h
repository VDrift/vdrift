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

#ifndef _LOADCOLLISIONSHAPE_H
#define _LOADCOLLISIONSHAPE_H

class PTree;
class btTransform;
class btCollisionShape;
class btCompoundShape;

// if compound not null capsule is added to compound
// if compound null and capsule non centered new compound is created
void LoadCapsuleShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound);

// if compound not null box is added to compound
// if compound null and box non centered new compound is created
void LoadBoxShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound);

// if compound not null hull is added to compound
void LoadHullShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound);

// if compound not null shape is added to compound
// if compound null but reqiured new compound is created
void LoadCollisionShape(
	const PTree & cfg,
	const btTransform & transform,
	btCollisionShape *& shape,
	btCompoundShape *& compound);

#endif //_LOADCOLLISIONSHAPE_H
