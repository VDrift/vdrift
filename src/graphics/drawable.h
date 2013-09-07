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
#include "memory.h"

class Model;
class Texture;
class VertexArray;
class StringIdMap;

class Drawable
{
public:
	Drawable();

	bool operator < (const Drawable & other) const;

	unsigned GetDrawList() const;
	void SetModel(const Model & model);

	const Texture * GetDiffuseMap() const;
	void SetDiffuseMap(const std::tr1::shared_ptr<Texture> & value);

	const Texture * GetMiscMap1() const;
	void SetMiscMap1(const std::tr1::shared_ptr<Texture> & value);

	const Texture * GetMiscMap2() const;
	void SetMiscMap2(const std::tr1::shared_ptr<Texture> & value);

	const VertexArray * GetVertArray() const;
	void SetVertArray(const VertexArray * value);

	/// draw vertex array as line segments if size > 0
	float GetLineSize() const;
	void SetLineSize(float size);

	const Mat4 & GetTransform() const;
	void SetTransform(const Mat4 & value);

	/// bounding sphere frustum culling
	const Vec3 & GetObjectCenter() const;
	void SetObjectCenter(const Vec3 & value);

	/// bounding sphere frustum culling
	float GetRadius() const;
	void SetRadius(float value);

	const Vec4 & GetColor() const;
	void SetColor(float nr, float ng, float nb, float na);
	void SetColor(float nr, float ng, float nb);
	void SetAlpha(float na);

	float GetDrawOrder() const;
	void SetDrawOrder(float value);

	bool GetDecal() const;
	void SetDecal(bool value);

	bool GetDrawEnable() const;
	void SetDrawEnable(bool value);

	bool GetCull() const;
	bool GetCullFront() const;
	void SetCull(bool newcull, bool newcullfront);

	/// this gets called if we are using the GL3 renderer
	/// it returns a reference to the RenderModelExternal structure
	RenderModelExt & GenRenderModelData(StringIdMap & stringMap);

	void SetVertexArrayObject(GLuint vao, unsigned int elementCount);

private:
	std::tr1::shared_ptr<Texture> diffuse_map;
	std::tr1::shared_ptr<Texture> misc_map1;
	std::tr1::shared_ptr<Texture> misc_map2;
	unsigned list_id;
	const VertexArray * vert_array;
	float linesize;
	Mat4 transform;
	Vec3 objcenter;
	float radius;
	Vec4 color;
	float draw_order;
	bool decal;
	bool drawenabled;
	bool cull;
	bool cull_front;

	bool texturesChanged;
	bool uniformsChanged;

	RenderModelExtDrawable renderModel;
};

inline bool Drawable::operator < (const Drawable & other) const
{
	return draw_order < other.draw_order;
}

inline unsigned Drawable::GetDrawList() const
{
	return list_id;
}

inline const Texture * Drawable::GetDiffuseMap() const
{
	return diffuse_map.get();
}

inline const Texture * Drawable::GetMiscMap1() const
{
	return misc_map1.get();
}

inline const Texture * Drawable::GetMiscMap2() const
{
	return misc_map2.get();
}

inline const VertexArray * Drawable::GetVertArray() const
{
	return vert_array;
}

inline float Drawable::GetLineSize() const
{
	return linesize;
}

inline const Mat4 & Drawable::GetTransform() const
{
	return transform;
}

inline const Vec3 & Drawable::GetObjectCenter() const
{
	return objcenter;
}

inline float Drawable::GetRadius() const
{
	return radius;
}

inline const Vec4 & Drawable::GetColor() const
{
	return color;
}

inline float Drawable::GetDrawOrder() const
{
	return draw_order;
}

inline bool Drawable::GetDecal() const
{
	return decal;
}

inline bool Drawable::GetDrawEnable() const
{
	return drawenabled;
}

inline void Drawable::SetDrawEnable(bool value)
{
	drawenabled = value;
}

inline bool Drawable::GetCull() const
{
	return cull;
}

inline bool Drawable::GetCullFront() const
{
	return cull_front;
}

#endif // _DRAWABLE_H
