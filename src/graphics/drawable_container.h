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

#ifndef _DRAWABLE_CONTAINER_H
#define _DRAWABLE_CONTAINER_H

#include "drawable.h"
#include "reseatable_reference.h"

// drawable container helper functions
namespace DrawableContainerHelper
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
template <typename DrawableType, typename ContainerType, bool use_transform>
void AddDrawableToContainer(DrawableType & drawable, ContainerType & container, const Mat4 & transform)
{
	if (drawable.GetDrawEnable())
	{
		if (use_transform)
			drawable.SetTransform(transform);
		container.push_back(&drawable);
	}
}
/// adds elements from the first container to the second
template <typename DrawableType, typename ContainerType, typename U, bool use_transform>
void AddDrawablesToContainer(ContainerType & source, U & dest, const Mat4 & transform)
{
	for (typename ContainerType::iterator i = source.begin(); i != source.end(); i++)
	{
		AddDrawableToContainer<DrawableType, U, use_transform>(*i, dest, transform);
	}
}
}

template <template <typename U> class Container>
struct DrawableContainer
{
	// you can add new drawable containers by modifying drawables list
	// see http://en.wikipedia.org/wiki/C_preprocessor#X-Macros
	#define DRAWABLES_LIST\
		X(normal_noblend)\
		X(normal_noblend_nolighting)\
		X(car_noblend)\
		X(normal_blend)\
		X(skybox_blend)\
		X(skybox_noblend)\
		X(text)\
		X(twodim)\
		X(particle)\
		X(lights_emissive)\
		X(lights_omni)\
		X(debug_lines)

	#define X(Y) Container <Drawable> Y;
	DRAWABLES_LIST
	#undef X

	template <typename T>
	void ForEach(T func)
	{
		#define X(Y) func(Y);
		DRAWABLES_LIST
		#undef X
	}

	template <typename T>
	void ForEach(T func) const
	{
		#define X(Y) func(Y);
		DRAWABLES_LIST
		#undef X
	}

	/// adds elements from the first drawable container to the second
	template <template <typename UU> class ContainerU, bool use_transform>
	void AppendTo(DrawableContainer <ContainerU> & dest, const Mat4 & transform)
	{
		#define X(Y) DrawableContainerHelper::AddDrawablesToContainer<Drawable, Container<Drawable>, ContainerU<Drawable>, use_transform> (Y, dest.Y, transform);
		DRAWABLES_LIST
		#undef X
	}

	/// this is slow, don't do it often
	reseatable_reference <Container <Drawable> > GetByName(const std::string & name)
	{
		reseatable_reference <Container <Drawable> > ref;
		#define X(Y) if (name == #Y) return Y;
		DRAWABLES_LIST
		#undef X
		return ref;
	}

	/// apply this functor to all drawables
	template <typename T>
	void ForEachDrawable(T func)
	{
		ForEach(DrawableContainerHelper::ApplyFunctor<T>(func));
	}

	bool empty() const
	{
		return (size() == 0);
	}

	unsigned int size() const
	{
		unsigned int count = 0;
		ForEach(DrawableContainerHelper::AccumulateSize(count));
		return count;
	}

	void clear()
	{
		ForEach(DrawableContainerHelper::ClearContainer());
	}

	void SetVisibility(bool newvis)
	{
		ForEach(DrawableContainerHelper::SetVisibility(newvis));
	}

	void SetAlpha(float a)
	{
		ForEach(DrawableContainerHelper::SetAlpha(a));
	}

	#undef DRAWABLES_LIST
};

#endif // _DRAWABLE_CONTAINER_H
