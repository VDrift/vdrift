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
