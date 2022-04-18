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

#include "drawable.h"
#include "texture.h"
#include "model.h"
#include <cmath>

Drawable::Drawable() :
	vert_array(NULL),
	model(NULL),
	center(0),
	radius(1E6),
	color(1),
	draw_order(0),
	decal(false),
	drawenabled(true),
	cull(false),
	textures_changed(true),
	uniforms_changed(true)
{
	tex_id[0] = 0;
	tex_id[1] = 0;
	tex_id[2] = 0;
}

void Drawable::SetTextures(unsigned id0, unsigned id1, unsigned id2)
{
	tex_id[0] = id0;
	tex_id[1] = id1;
	tex_id[2] = id2;
	textures_changed = true;
}

void Drawable::SetVertArray(const VertexArray * value)
{
	vert_array = value;
}

void Drawable::SetTransform(const Mat4 & value)
{
	transform = value;
	if (model)
	{
		center = model->GetAabb().GetCenter();
		transform.TransformVectorOut(center[0], center[1], center[2]);
	}
	else
	{
		center.Set(transform[12], transform[13], transform[14]);
	}
	uniforms_changed = true;
}


void Drawable::SetColor(float r, float g, float b, float a)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
	uniforms_changed = true;
}

void Drawable::SetColor(float r, float g, float b)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	uniforms_changed = true;
}

void Drawable::SetAlpha(float a)
{
	color[3] = a;
	uniforms_changed = true;
}

void Drawable::SetDrawOrder(float value)
{
	draw_order = value;
}

void Drawable::SetDecal(bool value)
{
	decal = value;
	uniforms_changed = true;
}

RenderModelExt & Drawable::GenRenderModelData(const DrawableAttributes & draw_attribs)
{
	// copy data over to the GL3V render_model object
	// eventually this should only be done when we update the values, but for now
	// we call this every time we draw the drawable

	// textures
	if (textures_changed)
	{
		render_model.clearTextureCache();
		render_model.textures.clear();
		if (tex_id[0])
		{
			render_model.textures.push_back(RenderTextureEntry(draw_attribs.tex0, tex_id[0], GL_TEXTURE_2D));
		}
		if (tex_id[1])
		{
			render_model.textures.push_back(RenderTextureEntry(draw_attribs.tex1, tex_id[1], GL_TEXTURE_2D));
		}
		if (tex_id[2])
		{
			render_model.textures.push_back(RenderTextureEntry(draw_attribs.tex2, tex_id[2], GL_TEXTURE_2D));
		}

		textures_changed = false;
	}

	// uniforms
	if (uniforms_changed)
	{
		render_model.clearUniformCache();
		render_model.uniforms.clear();

		// only add it if it's not the identity matrix
		if (transform != Mat4())
			render_model.uniforms.push_back(RenderUniformEntry(draw_attribs.transform, transform.GetArray(), 16));

		// only add it if it's not the default
		if (color != Vec4(1))
		{
			float srgba[4];
			srgba[0] = color[0] < 1 ? std::pow(color[0], 2.2f) : color[0];
			srgba[1] = color[1] < 1 ? std::pow(color[1], 2.2f) : color[1];
			srgba[2] = color[2] < 1 ? std::pow(color[2], 2.2f) : color[2];
			srgba[3] = color[3];
			render_model.uniforms.push_back(RenderUniformEntry(draw_attribs.color, srgba, 4));
		}

		uniforms_changed = false;
	}

	render_model.SetVertData(vsegment);

	return render_model;
}

void Drawable::SetModel(Model & newmodel)
{
	model = &newmodel;
	radius = newmodel.GetAabb().GetRadius();
	center = newmodel.GetAabb().GetCenter();
	transform.TransformVectorOut(center[0], center[1], center[2]);
}
