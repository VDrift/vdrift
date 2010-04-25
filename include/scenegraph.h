
#ifndef _SCENEGRAPH_H
#define _SCENEGRAPH_H

#include <string>
#include <list>
#include <map>
#include <vector>
#include <cassert>

#include "quaternion.h"
#include "mathvector.h"
#include "matrix4.h"
#include "vertexarray.h"
#include "keyed_container.h"
#include "containeralgorithm.h"

class MODEL;
class TEXTURE_GL;

class DRAWABLE
{
private:
	std::vector <int> list_ids;
	const TEXTURE_GL * diffuse_map;
	const TEXTURE_GL * misc_map1;
	const TEXTURE_GL * additive_map1;
	const TEXTURE_GL * additive_map2;
	const VERTEXARRAY * vert_array;
	std::vector <MATHVECTOR <float, 3> > lineverts;
	
	bool decal;
	float r,g,b,a;
	bool lit;
	bool drawenabled;
	bool is2d;
	bool partial_transparency;
	bool cull;
	bool cull_front;
	float radius; ///<for bounding sphere frustum culling
	float draw_order;
	bool blur;
	bool skybox;
	bool vertical_track; ///<used for so called "vertical tracking skyboxes"
	bool self_illumination; ///< when true, the additive map will be applied
	bool issmoke;
	bool distance_field;
	bool cameratransform;
	MATHVECTOR <float, 3> objcenter; ///<used for bounding sphere frustum culling
	float linesize;
	bool forcealphatest;
	
	MATRIX4 <float> transform;

public:
	DRAWABLE() : diffuse_map(NULL),misc_map1(NULL),
		 additive_map1(NULL),additive_map2(NULL),vert_array(NULL),decal(false),r(1.0),g(1.0),b(1.0),a(1.0),lit(true),drawenabled(true),
		is2d(false),partial_transparency(false),cull(false),cull_front(false),radius(0.0),
		draw_order(0),blur(true),
		skybox(false),vertical_track(false),self_illumination(false),issmoke(false),distance_field(false),
		cameratransform(true),objcenter(0), linesize(1.0), forcealphatest(false)
		{
		}
	
	bool operator< (const DRAWABLE & other) const;

	void SetTransform(const MATRIX4 <float> & trans) {transform = trans;}
	const MATRIX4 <float> & GetTransform() {return transform;}
	void SetDecal(bool newdecal) {decal = newdecal;}
	void Set2D(bool new2d) {is2d = new2d;}
	bool Get2D() const {return is2d;}
	void SetColor(float nr, float ng, float nb, float na) {r=nr;g=ng;b=nb;a=na;}
	void SetColor(float nr, float ng, float nb) {r=nr;g=ng;b=nb;}
	void SetAlpha(float na) {a=na;}
	void GetColor(float &nr, float &ng, float &nb, float &na) const {nr=r;ng=g;nb=b;na=a;}
	void SetLit(bool newlit) {lit = newlit;}
	bool GetLit() const {return lit;}
	void SetDrawEnable(bool newenable) {drawenabled = newenable;}
	void SetPartialTransparency(bool newpt) {partial_transparency = newpt;}
	bool GetPartialTransparency() const {return partial_transparency;}
	void SetCull(bool newcull, bool newcullfront) {cull = newcull; cull_front = newcullfront;}
	bool GetCull() const {return cull;}
	bool GetCullFront() const {return cull_front;}
	void SetRadius(float newrad) {radius = newrad;} //is about object center
	inline float GetRadius() const {return radius;}
	bool GetDrawEnable() const {return drawenabled;}
	void SetSmoke(bool newsmoke) {issmoke = newsmoke;}
	bool GetSmoke() const {return issmoke;}
	int GetOrder() const {return draw_order;}
	void SetBlur(bool newblur) {blur=newblur;}
	void SetSkybox(bool newskybox) {skybox = newskybox;}
	inline bool GetSkybox() const {return skybox;}
	bool GetDecal() const {return decal;}
	void SetSelfIllumination(bool newsi) {self_illumination=newsi;}
	
	void SetDrawOrder(float neworder) {draw_order = neworder;}
	
	void SetDrawOrderSmoke() {SetDrawOrder(1);}
	void SetDrawOrderFont() {SetDrawOrder(5);}
	void SetDrawOrderCursor() {SetDrawOrder(10);}
	void SetDrawOrderGUIBackground() {SetDrawOrder(2);}
	void SetDrawOrderGUIForeground() {SetDrawOrder(4);}

	void SetDiffuseMap ( const TEXTURE_GL* value )
	{
		diffuse_map = value;
	}

	const TEXTURE_GL* GetDiffuseMap() const
	{
		return diffuse_map;
	}

	void AddDrawList(int newlist)
	{
		list_ids.push_back(newlist);
	}
	
	const std::vector <int> & GetDrawLists() const
	{
		return list_ids;
	}
	
	bool IsDrawList() const {return !list_ids.empty();}

	void SetObjectCenter ( const MATHVECTOR< float, 3 >& value )
	{
		objcenter = value;
	}

	inline const MATHVECTOR< float, 3 > & GetObjectCenter() const
	{
		return objcenter;
	}

	void SetVertArray ( const VERTEXARRAY* value )
	{
		vert_array = value;
	}
	
	const VERTEXARRAY* GetVertArray() const
	{
		return vert_array;
	}

	void SetDistanceField ( bool value )
	{
		distance_field = value;
	}
	

	bool GetDistanceField() const
	{
		return distance_field;
	}

	void SetVerticalTrack ( bool value )
	{
		vertical_track = value;
	}

	bool GetVerticalTrack() const
	{
		return vertical_track;
	}

	///true for normal objects; if false camera transform and orientation aren't applied when the object is rendered
	void SetCameraTransformEnable ( bool value )
	{
		cameratransform = value;
	}

	bool GetCameraTransformEnable() const
	{
		return cameratransform;
	}

	void SetMiscMap1 ( const TEXTURE_GL* value )
	{
		misc_map1 = value;
	}

	void SetAdditiveMap1 ( const TEXTURE_GL* value )
	{
		additive_map1 = value;
	}

	void SetAdditiveMap2 ( const TEXTURE_GL* value )
	{
		additive_map2 = value;
	}

	const TEXTURE_GL* GetMiscMap1() const
	{
		return misc_map1;
	}

	const TEXTURE_GL* GetAdditiveMap1() const
	{
		return additive_map1;
	}

	const TEXTURE_GL* GetAdditiveMap2() const
	{
		return additive_map2;
	}

	bool GetSelfIllumination() const
	{
		return self_illumination;
	}

	float GetDrawOrder() const
	{
		return draw_order;
	}

	void SetLinesize ( float theValue )
	{
		linesize = theValue;
	}
	

	float GetLinesize() const
	{
		return linesize;
	}
	
	void AddLinePoint(const MATHVECTOR <float, 3> & newpoint)
	{
		lineverts.push_back(newpoint);
	}
	
	void ClearLine()
	{
		lineverts.clear();
	}

	const std::vector< MATHVECTOR < float , 3 > > & GetLine() const
	{
		return lineverts;
	}

	void SetForceAlphaTest ( bool theValue )
	{
		forcealphatest = theValue;
	}

	bool GetForceAlphaTest() const
	{
		return forcealphatest;
	}
};

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
};

template <typename T>
class PTRVECTOR : public std::vector <T*>
{};

template <template <typename U> class CONTAINER>
struct DRAWABLE_CONTAINER
{
	// all of the layers of the scene
	CONTAINER <DRAWABLE> generic;
	CONTAINER <DRAWABLE> twodim;
	CONTAINER <DRAWABLE> normal_noblend;
	CONTAINER <DRAWABLE> normal_blend;
	CONTAINER <DRAWABLE> skybox_blend;
	CONTAINER <DRAWABLE> skybox_noblend;
	CONTAINER <DRAWABLE> text;
	CONTAINER <DRAWABLE> particle;
	CONTAINER <DRAWABLE> nocamtrans_blend;
	CONTAINER <DRAWABLE> nocamtrans_noblend;
	// don't forget to add new members to the ForEach function and the AppendTo function
	
	template <typename T> 
	void ForEach(T func)
	{
		func(generic);
		func(twodim);
		func(normal_noblend);
		func(normal_blend);
		func(skybox_blend);
		func(skybox_noblend);
		func(text);
		func(particle);
		func(nocamtrans_blend);
		func(nocamtrans_noblend);
	}
	
	/// adds elements from the first drawable container to the second
	template <typename CONTAINERU, bool use_transform>
	void AppendTo(CONTAINERU & dest, const MATRIX4 <float> & transform)
	{
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(generic, dest.generic, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(twodim, dest.twodim, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(normal_noblend, dest.normal_noblend, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(normal_blend, dest.normal_blend, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(skybox_blend, dest.skybox_blend, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(skybox_noblend, dest.skybox_noblend, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(text, dest.text, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(particle, dest.particle, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(nocamtrans_blend, dest.nocamtrans_blend, transform);
		DRAWABLE_CONTAINER_HELPER::AddDrawablesToContainer<DRAWABLE,CONTAINER<DRAWABLE>,PTRVECTOR<DRAWABLE>,use_transform>
			(nocamtrans_noblend, dest.nocamtrans_noblend, transform);
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

class SCENETRANSFORM
{
private:
	typedef QUATERNION<float> QUAT;
	typedef MATHVECTOR<float,3> VEC3;
	
	QUAT rotation;
	VEC3 translation;

public:
	const QUAT & GetRotation() const {return rotation;}
	const VEC3 & GetTranslation() const {return translation;}
	void SetRotation(const QUAT & rot) {rotation = rot;}
	void SetTranslation(const VEC3 & trans) {translation = trans;}
	bool IsIdentityTransform() const {return (rotation == QUAT() && translation == VEC3());}
	void Clear() {rotation.LoadIdentity();translation.Set(0.0f);}
};

class STATICDRAWABLES
{
	private:
		typedef MATRIX4<float> MAT4;
		DRAWABLE_CONTAINER <keyed_container> drawlist;
		SCENETRANSFORM transform;
		
	public:
		DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() {return drawlist;}
		const DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() const {return drawlist;}
		SCENETRANSFORM & GetTransform() {return transform;}
};

class SCENENODE
{
private:
	typedef MATRIX4<float> MAT4;
	typedef MATHVECTOR<float,3> VEC3;
	
	keyed_container <SCENENODE> childlist;
	DRAWABLE_CONTAINER <keyed_container> drawlist;
	SCENETRANSFORM transform;
	MAT4 cached_transform;

public:
	keyed_container <SCENENODE>::handle AddNode() {return childlist.insert(SCENENODE());}
	SCENENODE & GetNode(keyed_container <SCENENODE>::handle handle) {return childlist.get(handle);}
	const SCENENODE & GetNode(keyed_container <SCENENODE>::handle handle) const {return childlist.get(handle);}
	
	DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() {return drawlist;}
	const DRAWABLE_CONTAINER <keyed_container> & GetDrawlist() const {return drawlist;}
	
	SCENETRANSFORM & GetTransform() {return transform;}
	const SCENETRANSFORM & GetTransform() const {return transform;}
	unsigned int Nodes() const {return childlist.size();}
	unsigned int Drawables() const {return drawlist.size();}
	void Traverse(DRAWABLE_CONTAINER <PTRVECTOR> & drawlist_output, const MAT4 & prev_transform);
	void Clear() {drawlist.clear();childlist.clear();}
	void Delete(keyed_container <SCENENODE>::handle handle) {childlist.erase(handle);}
	VEC3 TransformIntoWorldSpace() const {VEC3 zero;return TransformIntoWorldSpace(zero);}
	VEC3 TransformIntoWorldSpace(const VEC3 & localspace) const;
	VEC3 TransformIntoLocalSpace(const VEC3 & worldspace) const;
	void SetChildVisibility(bool newvis);
	void SetChildAlpha(float a);
	void DebugPrint(std::ostream & out, int curdepth = 0);
	
	/// traverse all drawable containers applying the specified functor.
	/// the functor should take a drawable container reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableContainerFunctor(T functor)
	{
		functor(drawlist);
		for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->ApplyDrawableContainerFunctor(functor);
		}
	}
	
	/// traverse all drawables applying the specified functor.
	/// the functor should take any drawable typed reference as an argument.
	/// note that the functor is passed by value to this function.
	template <typename T>
	void ApplyDrawableFunctor(T functor)
	{
		drawlist.ForEachDrawable(functor);
		for (keyed_container <SCENENODE>::iterator i = childlist.begin(); i != childlist.end(); ++i)
		{
			i->ApplyDrawableFunctor(functor);
		}
	}
};

#endif
