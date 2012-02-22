#ifndef _LOADDRAWABLE_H
#define _LOADDRAWABLE_H

#include "scenenode.h"

class ContentManager;
class PTree;

// Load drawable functor, returns false on error.
//
// [foo]
// texture = diff.png, spec.png, norm.png		#required
// mesh = model.joe								#required
// position = 0.736, 1.14, -0.47				#optional relative to parent
// rotation = 0, 0, 30							#optional relative to parent
// scale = -1, 1, 1								#optional
// color = 0.8, 0.1, 0.1						#optional color rgb
// draw = transparent							#optional type (transparent, emissive)
//
struct LoadDrawable
{
	const std::string & path;
	const int anisotropy;
	ContentManager & content;
	std::list<std::tr1::shared_ptr<MODEL> > & modellist;
	std::ostream & error;

	LoadDrawable(
		const std::string & path,
		const int anisotropy,
		ContentManager & content,
		std::list<std::tr1::shared_ptr<MODEL> > & modellist,
		std::ostream & error);

	bool operator()(
		const PTree & cfg,
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle * nodehandle = 0,
		keyed_container<DRAWABLE>::handle * drawhandle = 0);

	bool operator()(
		const std::string & meshname,
		const std::vector<std::string> & texname,
		const PTree & cfg,
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle * nodeptr = 0,
		keyed_container<DRAWABLE>::handle * drawptr = 0);
};

#endif // _LOADDRAWABLE_H
