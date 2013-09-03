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

#ifndef _DRAWABLE_H
#define _DRAWABLE_H

#include "rendermodelext_drawable.h"
#include "mathvector.h"
#include "matrix4.h"
#include "model.h"
#include "memory.h"

class Texture;
class VertexArray;
class GLWrapper;
class StringIdMap;

class Drawable
{
public:
	bool operator < (const Drawable & other) const {return draw_order < other.draw_order;}

	bool IsDrawList() const {return !list_ids.empty();}

	const std::vector <int> & GetDrawLists() const {return list_ids;}

	void SetModel(const Model & model);

	const Texture * GetDiffuseMap() const {return diffuse_map.get();}
	void SetDiffuseMap(const std::tr1::shared_ptr<Texture> & value);

	const Texture * GetMiscMap1() const {return misc_map1.get();}
	void SetMiscMap1(const std::tr1::shared_ptr<Texture> & value);

	const Texture * GetMiscMap2() const {return misc_map2.get();}
	void SetMiscMap2(const std::tr1::shared_ptr<Texture> & value);

	const VertexArray * GetVertArray() const {return vert_array;}
	void SetVertArray(const VertexArray* value);

	/// draw vertex array as line segments if size > 0
	void SetLineSize(float size) { linesize = size; renderModel.SetLineSize(size);}
	float GetLineSize() const { return linesize; }

	const Matrix4 <float> & GetTransform() const {return transform;}
	void SetTransform(const Matrix4 <float> & value);

	/// used for bounding sphere frustum culling
	const Vec3 & GetObjectCenter() const {return objcenter;}
	void SetObjectCenter(const Vec3 & value) {objcenter = value;}

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

	/// this gets called if we are using the GL3 renderer
	/// it returns a reference to the RenderModelExternal structure
	RenderModelExt & generateRenderModelData(StringIdMap & stringMap);

	void setVertexArrayObject(GLuint vao, unsigned int elementCount);

	Drawable() :
		vert_array(0),
		linesize(0),
		radius(0),
		r(1), g(1), b(1), a(1),
		draw_order(0),
		decal(false),
		drawenabled(true),
		cull(false),
		cull_front(false),
		texturesChanged(true),
		uniformsChanged(true)
	{
		objcenter.Set(0.0);
	}

private:
	std::tr1::shared_ptr<Texture> diffuse_map;
	std::tr1::shared_ptr<Texture> misc_map1;
	std::tr1::shared_ptr<Texture> misc_map2;
	std::vector <int> list_ids;
	const VertexArray * vert_array;
	float linesize;
	Matrix4 <float> transform;
	Vec3 objcenter;
	float radius;
	float r, g, b, a; // these can never be reordered or interleaved with other data because we count on (&a == &r+3) and so on
	float draw_order;
	bool decal;
	bool drawenabled;
	bool cull;
	bool cull_front;

	bool texturesChanged;
	bool uniformsChanged;

	RenderModelExtDrawable renderModel;

	void AddDrawList(int value) {list_ids.push_back(value);}
};

#endif // _DRAWABLE_H
