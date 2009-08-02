
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

class MODEL;
class TEXTURE_GL;
class SCENENODE;
class DRAWABLE;

class DRAWABLE_FILTER
{
	private:
		enum FILTERTYPE
		{
			IS2D=0,
			PARTIALTRANSPARENCY=1,
			BLUR=2,
			SKYBOX=3,
			CAMERATRANSFORM=4
		};
	
		unsigned int filtermask;
		unsigned int filtervalue;
	
		unsigned int SetBit(unsigned int input, unsigned int bit) const
		{
			return input | (1 << bit);
		}
	
		unsigned int UnsetBit(unsigned int input, unsigned int bit) const
		{
			return input & ~(1 << bit);
		}
	
		bool GetBit(unsigned int input, unsigned int bit) const
		{
			return ((input & (1 << bit)) != 0);
		}
	
		void SetFilter(FILTERTYPE type, bool filter_enable, bool filter_value)
		{
			unsigned int bit = type;
			
			filtermask = filter_enable ? SetBit(filtermask, bit) : UnsetBit(filtermask, bit);
			filtervalue = filter_value ? SetBit(filtervalue, bit) : UnsetBit(filtervalue, bit);
		}
	
	public:
		DRAWABLE_FILTER() : filtermask(0),filtervalue(0) {}
	
		bool Is2DOnlyFilter() {return GetBit(filtermask, IS2D) && GetBit(filtervalue, IS2D);}
	
		void SetFilter_is2d(bool filter_enable, bool filter_value) {SetFilter(IS2D, filter_enable, filter_value);}
		void SetFilter_partial_transparency(bool filter_enable, bool filter_value) {SetFilter(PARTIALTRANSPARENCY, filter_enable, filter_value);}
		void SetFilter_blur(bool filter_enable, bool filter_value) {SetFilter(BLUR, filter_enable, filter_value);}
		void SetFilter_skybox(bool filter_enable, bool filter_value) {SetFilter(SKYBOX, filter_enable, filter_value);}
		void SetFilter_cameratransform(bool filter_enable, bool filter_value) {SetFilter(CAMERATRANSFORM, filter_enable, filter_value);}
		bool Matches(const DRAWABLE & drawable) const;
		bool Allows2D() const {return !(GetBit(filtermask, IS2D) && !GetBit(filtervalue, IS2D));}
		bool AllowsSkybox() const {return !(GetBit(filtermask, SKYBOX) && !GetBit(filtervalue, SKYBOX));}
		bool AllowsNoCameraTransform() const {return !(GetBit(filtermask, CAMERATRANSFORM) && GetBit(filtervalue, CAMERATRANSFORM));}
};

class DRAWABLE
{
friend class DRAWABLE_FILTER;
private:
	SCENENODE * parent;

	std::vector <int> list_ids;
	const TEXTURE_GL * diffuse_map;
	const TEXTURE_GL * misc_map1;
	const TEXTURE_GL * additive_map1;
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
	
	DRAWABLE_FILTER filterspeedup; ///< a bitmask that duplicates some flags above to speed up drawable_filter matching

public:
	DRAWABLE() : parent(NULL),diffuse_map(NULL),misc_map1(NULL),
		 additive_map1(NULL),vert_array(NULL),decal(false),r(1.0),g(1.0),b(1.0),a(1.0),lit(true),drawenabled(true),
		is2d(false),partial_transparency(false),cull(false),cull_front(false),radius(0.0),
		draw_order(0),blur(true),
		skybox(false),vertical_track(false),self_illumination(false),issmoke(false),distance_field(false),
		cameratransform(true),objcenter(0), linesize(1.0), forcealphatest(false)
		{
			filterspeedup.SetFilter_blur(true, true);
			filterspeedup.SetFilter_cameratransform(true, true);
		}
	~DRAWABLE();
	
	bool operator< (const DRAWABLE & other) const;

	void Set(const DRAWABLE & other) {*this = other;}
	void SetParent(SCENENODE & newparent) {parent = &newparent;}
	SCENENODE * GetParent() const {return parent;}
	void SetDecal(bool newdecal) {decal = newdecal;}
	void Set2D(bool new2d) {is2d = new2d;filterspeedup.SetFilter_is2d(true, new2d);}
	bool Get2D() const {return is2d;}
	void SetColor(float nr, float ng, float nb, float na) {r=nr;g=ng;b=nb;a=na;}
	void SetColor(float nr, float ng, float nb) {r=nr;g=ng;b=nb;}
	void SetAlpha(float na) {a=na;}
	void GetColor(float &nr, float &ng, float &nb, float &na) const {nr=r;ng=g;nb=b;na=a;}
	void SetLit(bool newlit) {lit = newlit;}
	bool GetLit() const {return lit;}
	void SetDrawEnable(bool newenable) {drawenabled = newenable;}
	void SetPartialTransparency(bool newpt) {partial_transparency = newpt;filterspeedup.SetFilter_partial_transparency(true, newpt);}
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
	void SetBlur(bool newblur) {blur=newblur;filterspeedup.SetFilter_blur(true, newblur);}
	void SetSkybox(bool newskybox) {skybox = newskybox;filterspeedup.SetFilter_skybox(true, newskybox);}
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
		filterspeedup.SetFilter_cameratransform(true, value);
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

	const TEXTURE_GL* GetMiscMap1() const
	{
		return misc_map1;
	}

	const TEXTURE_GL* GetAdditiveMap1() const
	{
		return additive_map1;
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

class SCENETRANSFORM
{
private:
	typedef QUATERNION<float> QUAT;
	typedef MATHVECTOR<float,3> VEC3;
	
	QUAT rotation;
	VEC3 translation;
	
	//used for tweening
	VEC3 angular_velocity;
	VEC3 linear_velocity;

public:
	const QUAT & GetRotation() const {return rotation;}
	const VEC3 & GetTranslation() const {return translation;}
	void SetRotation(const QUAT & rot) {rotation = rot;}
	void SetTranslation(const VEC3 & trans) {translation = trans;}
	bool IsIdentityTransform() const {return (rotation == QUAT() && translation == VEC3());}
	void Clear() {rotation.LoadIdentity();translation.Set(0.0f);}
};

class SCENEDRAW
{
private:
	//const SCENETRANSFORM * transform;
	const DRAWABLE * draw;
	//VERTEXARRAY varray; //unfortunately (for text and racing line drawing) the main thread will change this, so we need a copy //edit: way faster without this, so who cares about multithreading, for now
	
	typedef MATRIX4<float> MAT4;
	MAT4 mat4;

public:
	SCENEDRAW() : draw(NULL) {}
	SCENEDRAW(const SCENEDRAW & other) : draw(other.draw),/*varray(other.varray),*/mat4(other.mat4)
	{
		//if (draw.GetVertArray()) draw.SetVertArray(&varray);
	}
	SCENEDRAW & operator=(const SCENEDRAW & other)
	{
		draw = other.draw;
		mat4=other.mat4;
		//varray = other.varray;
		//if (draw.GetVertArray()) draw.SetVertArray(&varray);
		return *this;
	}
	SCENEDRAW(const DRAWABLE & newdraw, const MAT4 & newmat4) : draw(&newdraw), mat4(newmat4) {}
	bool operator< (const SCENEDRAW & other) const;
	bool IsCollapsed() const {return true;}
	bool IsDraw() const {return true;}
	const DRAWABLE * GetDraw() const {assert(draw);return draw;}
	const MAT4 * GetMatrix4() const {return &mat4;}
	void SetCollapsed(const DRAWABLE & newdraw, const MAT4 & newmat4)
	{
		draw = &newdraw;
		mat4.Set(newmat4);
		
		/*//detect if we need to buffer the vertex array
		if (draw.GetVertArray())
		{
			varray = *draw.GetVertArray();
			draw.SetVertArray(&varray);
		}*/
	}
};

class SCENENODE
{
private:
	std::list <SCENENODE> childlist;
	std::list <DRAWABLE> drawlist;
	SCENETRANSFORM transform;

	SCENENODE * parent;
	
	typedef MATRIX4<float> MAT4;
	typedef MATHVECTOR<float,3> VEC3;

public:
	SCENENODE() : parent(NULL) {}
	~SCENENODE();
	std::list <DRAWABLE> & GetDrawableList() {return drawlist;}
	SCENENODE & AddNode();
	DRAWABLE & AddDrawable();
	unsigned int Nodes() const {return childlist.size();}
	unsigned int Drawables() const {return drawlist.size();}
	SCENETRANSFORM & GetTransform() {return transform;}
	//void GetDrawList(std::vector <SCENEDRAW> & drawlistoutput, const DRAWABLE_FILTER & filter) const;
	void GetCollapsedDrawList(std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > & drawlist_output_map, const MAT4 & prev_transform) const;
	void SetParent(SCENENODE & newparent) {parent = &newparent;}
	SCENENODE * GetParent() {return parent;}
	void Clear() {drawlist.clear();childlist.clear();}
	void Delete(SCENENODE * todelete);
	void Delete(DRAWABLE * todelete);
	VEC3 TransformIntoWorldSpace() const {VEC3 zero;return TransformIntoWorldSpace(zero);}
	VEC3 TransformIntoWorldSpace(const VEC3 & localspace) const;
	VEC3 TransformIntoLocalSpace(const VEC3 & worldspace) const;
	MAT4 CollapseTransform() const;
	void DebugPrint(std::ostream & out);
	void SortDrawablesByRenderState();
	void SetChildVisibility(bool newvis);
	void SetChildAlpha(float a);
};

/*class SCENEGRAPH
{
private:
	SCENENODE rootnode;

public:
	SCENEGRAPH() {}
	~SCENEGRAPH() {}

	const TESTER Test();
	SCENENODE & GetRoot() {return rootnode;}
	//void GetDrawList(list <SCENEDRAW> & drawlist) const;
	void GetDrawList(list <DRAWABLE_FILTER *> & filter_list, map < DRAWABLE_FILTER *, list <SCENEDRAW> > & drawlist_output_map) const;
	//void GetCollapsedDrawList(list <SCENEDRAW> & drawlist) const;
	void GetCollapsedDrawList(list <DRAWABLE_FILTER *> & filter_list, map < DRAWABLE_FILTER *, list <SCENEDRAW> > & drawlist_output_map) const;
	void Delete(SCENENODE * todelete);
	void Delete(DRAWABLE * todelete);
};*/

#endif
