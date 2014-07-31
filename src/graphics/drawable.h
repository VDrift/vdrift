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
#include "vertexbuffer.h"
#include "mathvector.h"
#include "matrix4.h"

class Model;
class VertexArray;
class StringIdMap;

class Drawable
{
public:
	Drawable();

	bool operator < (const Drawable & other) const;

	unsigned GetTexture0() const;
	unsigned GetTexture1() const;
	unsigned GetTexture2() const;
	void SetTextures(unsigned id0, unsigned id1 = 0, unsigned id2 = 0);

	const VertexArray * GetVertArray() const;
	void SetVertArray(const VertexArray * value);

	const Mat4 & GetTransform() const;
	void SetTransform(const Mat4 & value);

	/// bounding sphere center in local space
	const Vec3 & GetObjectCenter() const;
	void SetObjectCenter(const Vec3 & value);

	/// bounding sphere radius
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
	void SetCull(bool newcull);

	/// this gets called if we are using the GL3 renderer
	/// returns a reference to the RenderModelExternal structure
	RenderModelExt & GenRenderModelData(StringIdMap & string_map);

	/// setting model will also set bounding sphere center and radius
	Model * GetModel() const;
	void SetModel(Model & newmodel);

	/// vertex buffer interface
	const VertexBuffer::Segment & GetVertexBufferSegment() const;
	void SetVertexBufferSegment(const VertexBuffer::Segment & segment);

private:
	unsigned tex_id[3];
	VertexBuffer::Segment vsegment;
	const VertexArray * vert_array;
	Model * model;

	Mat4 transform;
	Vec3 center;
	float radius;
	Vec4 color;
	float draw_order;
	bool decal;
	bool drawenabled;
	bool cull;

	bool textures_changed;
	bool uniforms_changed;
	RenderModelExtDrawable render_model;
};

inline bool Drawable::operator < (const Drawable & other) const
{
	return draw_order < other.draw_order;
}

inline unsigned Drawable::GetTexture0() const
{
	return tex_id[0];
}

inline unsigned Drawable::GetTexture1() const
{
	return tex_id[1];
}

inline unsigned Drawable::GetTexture2() const
{
	return tex_id[2];
}

inline const VertexArray * Drawable::GetVertArray() const
{
	return vert_array;
}

inline const Mat4 & Drawable::GetTransform() const
{
	return transform;
}

inline const Vec3 & Drawable::GetObjectCenter() const
{
	return center;
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

inline void Drawable::SetCull(bool newcull)
{
	cull = newcull;
}

inline bool Drawable::GetCull() const
{
	return cull;
}

inline Model * Drawable::GetModel() const
{
	return model;
}

inline const VertexBuffer::Segment & Drawable::GetVertexBufferSegment() const
{
	return vsegment;
}

inline void Drawable::SetVertexBufferSegment(const VertexBuffer::Segment & segment)
{
	vsegment = segment;
}

#endif // _DRAWABLE_H
