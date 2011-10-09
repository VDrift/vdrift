#ifndef _SKIDMARK_H
#define _SKIDMARK_H

#include "scenenode.h"
#include "vertexarray.h"

class ContentManager;

class SKIDMARK
{
public:
	/// strip_count: number of separate strips, cyclic buffer
	/// strip_length: number of quads per strip
	SKIDMARK(
		int strip_count = 128,
		int strip_length = 8,
		float strength_min = 0.6,
		float length_min = 0.1, // 0.3 meter
		float offset = 0.01);

	/// init drawable
	bool Init(
		ContentManager & content,
		const std::string & path,
		const std::string & texname);

	/// update a skid mark, id will be modified
	/// strength: skid mark strength 0-1, affects transparency
	void Update(
		unsigned int & id,
		const MATHVECTOR<float, 3> & position,
		const MATHVECTOR<float, 3> & left,
		const MATHVECTOR<float, 3> & normal,
		const float half_width,
		const float strength);

	/// call after updating skid marks and before drawing
	void Update();

	/// clear all skid marks
	void Reset();

	SCENENODE & GetNode() {return node;}

private:
	const unsigned int strip_count;
	const unsigned int strip_length;
	const float strength_min;
	const float length_min;
	const float offset;

	struct STRIP
	{
		keyed_container <DRAWABLE>::handle draw;
		VERTEXARRAY varray;
		MATHVECTOR<float, 3> position;
		MATHVECTOR<float, 3> left;
		MATHVECTOR<float, 3> normal;
		float hwidth;
		float strength;
		unsigned int length;

		STRIP();
	};
	SCENENODE node;
	std::vector<STRIP> strip;
	unsigned int strip_next;
	unsigned int strip_old;
};

#endif // _SKIDMARK_H
