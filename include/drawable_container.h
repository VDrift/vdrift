#ifndef _DRAWABLE_CONTAINER_H
#define _DRAWABLE_CONTAINER_H

#include "drawable.h"
#include "reseatable_reference.h"

// drawable container helper functions
namespace DRAWABLE_CONTAINER_HELPER
{
struct SetVisibility
{
	SetVisibility(bool newvis) : vis(newvis) {}
	bool vis;
	template <typename T>
	void operator()(T & container)
	{
		for (typename T::iterator i = container.begin(); i != container.end(); i++)
		{
			i->SetDrawEnable(vis);
		}
	}
};
struct SetAlpha
{
	SetAlpha(float newa) : a(newa) {}
	float a;
	template <typename T>
	void operator()(T & container)
	{
		for (typename T::iterator i = container.begin(); i != container.end(); i++)
		{
			i->SetAlpha(a);
		}
	}
};
struct ClearContainer
{
	template <typename T>
	void operator()(T & container)
	{
		container.clear();
	}
};
struct AccumulateSize
{
	AccumulateSize(unsigned int & newcount) : count(newcount) {}
	unsigned int & count;
	template <typename T>
	void operator()(const T & container)
	{
		count += container.size();
	}
};
template <typename F>
struct ApplyFunctor
{
	ApplyFunctor(F newf) : func(newf) {}
	F func;
	template <typename T>
	void operator()(T & container)
	{
		for (typename T::iterator i = container.begin(); i != container.end(); i++)
		{
			func(*i);
		}
	}
};
template <typename DRAWABLE_TYPE, typename CONTAINER_TYPE, bool use_transform>
void AddDrawableToContainer(DRAWABLE_TYPE & drawable, CONTAINER_TYPE & container, const MATRIX4 <float> & transform)
{
	if (drawable.GetDrawEnable())
	{
		if (use_transform)
			drawable.SetTransform(transform);
		container.push_back(&drawable);
	}
}
/// adds elements from the first container to the second
template <typename DRAWABLE_TYPE, typename CONTAINERT, typename U, bool use_transform>
void AddDrawablesToContainer(CONTAINERT & source, U & dest, const MATRIX4 <float> & transform)
{
	for (typename CONTAINERT::iterator i = source.begin(); i != source.end(); i++)
	{
		AddDrawableToContainer<DRAWABLE_TYPE,U,use_transform>(*i, dest, transform);
	}
}
}

/// pointer vector, drawable_container template parameter
template <typename T>
class PTRVECTOR : public std::vector<T*>
{};

template <template <typename U> class CONTAINER>
struct DRAWABLE_CONTAINER
{
	// all of the layers of the scene

	#define X(Y) CONTAINER <DRAWABLE> Y;
	#include "drawables.def"
	#undef X
	// you can add new drawable containers by modifying drawables.def
	// see http://en.wikipedia.org/wiki/C_preprocessor#X-Macros

	template <typename T>
	void ForEach(T func)
	{
		#define X(Y) func(Y);
		#include "drawables.def"
		#undef X
	}

	template <typename T>
	void ForEachWithName(T func)
	{
		#define X(Y) func(#Y,Y);
		#include "drawables.def"
		#undef X
	}

	/// adds elements from the first drawable container to the second
	template <template <typename UU> class CONTAINERU, bool use_transform>
	void AppendTo(DRAWABLE_CONTAINER <CONTAINERU> & dest, const MATRIX4 <float> & transform)
	{
		#define X(Y) DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,CONTAINERU<DRAWABLE>,use_transform> (Y, dest.Y, transform);
		#include "drawables.def"
		#undef X
	}

	/// this is slow, don't do it often
	reseatable_reference <CONTAINER <DRAWABLE> > GetByName(const std::string & name)
	{
		reseatable_reference <CONTAINER <DRAWABLE> > ref;
		#define X(Y) if (name == #Y) return Y;
		#include "drawables.def"
		#undef X
		return ref;
	}

	/// apply this functor to all drawables
	template <typename T>
	void ForEachDrawable(T func)
	{
		ForEach(DRAWABLE_CONTAINER_HELPER::ApplyFunctor<T>(func));
	}

	bool empty() const
	{
		return (size() == 0);
	}

	unsigned int size() const
	{
		DRAWABLE_CONTAINER <CONTAINER> * me = const_cast<DRAWABLE_CONTAINER <CONTAINER> *>(this); // messy, but avoids more typing. const correctness is enforced in AccumulateSize::operator()
		unsigned int count = 0;
		me->ForEach(DRAWABLE_CONTAINER_HELPER::AccumulateSize(count));
		return count;
	}

	void clear()
	{
		ForEach(DRAWABLE_CONTAINER_HELPER::ClearContainer());
	}

	void SetVisibility(bool newvis)
	{
		ForEach(DRAWABLE_CONTAINER_HELPER::SetVisibility(newvis));
	}

	void SetAlpha(float a)
	{
		ForEach(DRAWABLE_CONTAINER_HELPER::SetAlpha(a));
	}
};

#endif // _DRAWABLE_CONTAINER_H
