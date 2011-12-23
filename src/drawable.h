#ifndef _DRAWABLE_H
#define _DRAWABLE_H

#include "matrix4.h"
#include "mathvector.h"
#include "model.h"
#include "rendermodelext_drawable.h"
#include "memory.h"

class TEXTURE;
class VERTEXARRAY;
class GLWrapper;
class StringIdMap;

class DRAWABLE
{
public:
	bool operator < (const DRAWABLE & other) const {return draw_order < other.draw_order;}

	bool IsDrawList() const {return !list_ids.empty();}

	const std::vector <int> & GetDrawLists() const {return list_ids;}

	void SetModel(const MODEL & model);

	const TEXTURE * GetDiffuseMap() const {return diffuse_map.get();}
	void SetDiffuseMap(const std::tr1::shared_ptr<TEXTURE> & value);

	const TEXTURE * GetMiscMap1() const {return misc_map1.get();}
	void SetMiscMap1(const std::tr1::shared_ptr<TEXTURE> & value);

	const TEXTURE * GetMiscMap2() const {return misc_map2.get();}
	void SetMiscMap2(const std::tr1::shared_ptr<TEXTURE> & value);

	const VERTEXARRAY * GetVertArray() const {return vert_array;}
	void SetVertArray(const VERTEXARRAY* value);

	/// draw vertex array as line segments if size > 0
	void SetLineSize(float size) { linesize = size; }
	float GetLineSize() const { return linesize; }

	const MATRIX4 <float> & GetTransform() {return transform;}
	void SetTransform(const MATRIX4 <float> & value);

	/// used for bounding sphere frustum culling
	const MATHVECTOR <float, 3> & GetObjectCenter() const {return objcenter;}
	void SetObjectCenter(const MATHVECTOR <float, 3> & value) {objcenter = value;}

	/// for bounding sphere frustum culling
	float GetRadius() const {return radius;}
	void SetRadius(float value) {radius = value;}

	void GetColor(float &nr, float &ng, float &nb, float &na) const {nr = r; ng = g; nb = b; na = a;}
	void SetColor(float nr, float ng, float nb, float na);
	void SetColor(float nr, float ng, float nb);
	void SetAlpha(float na);

	float GetDrawOrder() const {return draw_order;}
	void SetDrawOrder(float value) {draw_order = value;}
	void SetDrawOrderFont() {SetDrawOrder(5);}
	void SetDrawOrderCursor() {SetDrawOrder(10);}
	void SetDrawOrderGUIBackground() {SetDrawOrder(2);}
	void SetDrawOrderGUIForeground() {SetDrawOrder(4);}

	bool GetDecal() const {return decal;}
	void SetDecal(bool newdecal) {decal = newdecal;uniformsChanged = true;}

	bool GetDrawEnable() const {return drawenabled;}
	void SetDrawEnable(bool value) {drawenabled = value;}

	bool GetCull() const {return cull;}
	bool GetCullFront() const {return cull_front;}
	void SetCull(bool newcull, bool newcullfront) {cull = newcull; cull_front = newcullfront;}

	bool GetSkybox() const {return skybox;}
	void SetSkybox(bool value) {skybox = value;}

	/// used for so called "vertical tracking skyboxes"
	bool GetVerticalTrack() const {return vertical_track;}
	void SetVerticalTrack(bool value) {vertical_track = value;}

	/// true for normal objects; if false camera transform and orientation aren't applied when the object is rendered
	bool GetCameraTransformEnable() const {return cameratransform;}
	void SetCameraTransformEnable(bool value) {cameratransform = value;}

	/// this gets called if we are using the GL3 renderer
	/// it returns a reference to the RenderModelExternal structure
	RenderModelExternal & generateRenderModelData(GLWrapper & gl, StringIdMap & stringMap);

	void setVertexArrayObject(GLuint vao, unsigned int elementCount);

	DRAWABLE() :
		vert_array(0),
		linesize(0),
		radius(0),
		r(1), g(1), b(1), a(1),
		draw_order(0),
		decal(false),
		drawenabled(true),
		cull(false),
		cull_front(false),
		skybox(false),
		vertical_track(false),
		cameratransform(true),
		texturesChanged(true),
		uniformsChanged(true)
	{
		objcenter.Set(0.0);
	}

private:
	std::tr1::shared_ptr<TEXTURE> diffuse_map;
	std::tr1::shared_ptr<TEXTURE> misc_map1;
	std::tr1::shared_ptr<TEXTURE> misc_map2;
	std::vector <int> list_ids;
	const VERTEXARRAY * vert_array;
	float linesize;
	MATRIX4 <float> transform;
	MATHVECTOR <float, 3> objcenter;
	float radius;
	float r, g, b, a; // these can never be reordered or interleaved with other data because we count on (&a == &r+3) and so on
	float draw_order;
	bool decal;
	bool drawenabled;
	bool cull;
	bool cull_front;
	bool skybox;
	bool vertical_track;
	bool cameratransform;

	bool texturesChanged;
	bool uniformsChanged;

	RenderModelExternalDrawable renderModel;

	void AddDrawList(int value) {list_ids.push_back(value);}
};

#endif // _DRAWABLE_H
