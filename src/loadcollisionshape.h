#ifndef _LOADCOLLISIONSHAPE_H
#define _LOADCOLLISIONSHAPE_H

class PTree;
class btVector3;
class btCollisionShape;

void LoadBoxShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape);

void LoadHullShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape);

void LoadCollisionShape(
	const PTree & cfg,
	const btVector3 & center,
	btCollisionShape *& shape);

#endif //_LOADCOLLISIONSHAPE_H
