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

#include "hudgauge.h"
#include "gui/text_draw.h"
#include "graphics/texture.h"
#include <cassert>
#include <sstream>

static SceneNode::DrawableHandle AddDrawable(SceneNode & node)
{
	return node.GetDrawList().twodim.insert(Drawable());
}

static Drawable & GetDrawable(SceneNode & node, const SceneNode::DrawableHandle & drawhandle)
{
	return node.GetDrawList().twodim.get(drawhandle);
}

static void EraseDrawable(SceneNode & node, SceneNode::DrawableHandle & drawhandle)
{
	if (drawhandle.valid())
	{
		node.GetDrawList().twodim.erase(drawhandle);
		drawhandle.invalidate();
	}
}

static SceneNode::DrawableHandle AddTextDrawable(SceneNode & node)
{
	return node.GetDrawList().twodim.insert(Drawable());
}

static Drawable & GetTextDrawable(SceneNode & node, const SceneNode::DrawableHandle & drawhandle)
{
	return node.GetDrawList().twodim.get(drawhandle);
}

static void EraseTextDrawable(SceneNode & node, SceneNode::DrawableHandle & drawhandle)
{
	if (drawhandle.valid())
	{
		node.GetDrawList().twodim.erase(drawhandle);
		drawhandle.invalidate();
	}
}

HudGauge::HudGauge() :
	centerx(0),
	centery(0),
	scalex(1),
	scaley(1),
	startangle(0),
	endangle(0),
	scale(1)
{
	// ctor
}

void HudGauge::Set(
	SceneNode & parent,
	const std::tr1::shared_ptr<Texture> & texture,
	const Font & font,
	float hwratio,
	float centerx,
	float centery,
	float radius,
	float startangle,
	float endangle,
	float startvalue,
	float endvalue,
	float valuedelta)
{
	assert(texture);

	// calculate number of segments (max 9)
	float segments = (endvalue - startvalue) / valuedelta;
	float factor = ceil(segments / 9.0f);
	segments = ceil(segments / factor);
	valuedelta = valuedelta * factor;
	endvalue = startvalue + segments * valuedelta;

	this->texture = texture;
	this->centerx = centerx;
	this->centery = centery;
	this->scalex = radius * hwratio;
	this->scaley = radius;
	this->startangle = startangle;
	this->endangle = endangle;
	this->scale = (endangle - startangle) / (endvalue - startvalue);

	// reset
	EraseTextDrawable(parent, dialnum_draw);
	EraseDrawable(parent, pointer_draw);
	EraseDrawable(parent, dial_draw);
	pointer_rotated.Clear();
	pointer.Clear();
	dial_label.Clear();
	dial_marks.Clear();

	// dial marks
	{
		// big marker
		float pb[] = {-0.02, 1, 0, 0.02, 1, 0, 0.02, 0.92, 0, -0.02, 0.92, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		unsigned int f[] = {0, 2, 1, 0, 3, 2};
		VertexArray bm;
		bm.Add(f, 6, pb, 12, t, 8);

		// small marker
		float ps[] = {-0.01, 1, 0, 0.01, 1, 0, 0.01, 0.95, 0, -0.01, 0.95, 0};
		VertexArray sm;
		sm.Add(f, 6, ps, 12, t, 8);

		float delta = (endangle - startangle) / (3.0 * segments);
		float angle = startangle;
		for (int i = 0; i <= 3 * segments; ++i)
		{
			VertexArray temp = (i % 3) ? sm : bm;
			temp.Rotate(angle, 0, 0, -1);
			dial_marks = dial_marks + temp;
			angle = angle + delta;
		}
		dial_marks.Scale(radius * hwratio, radius, 1);
		dial_marks.Translate(centerx, centery, 0.0);

		dial_draw = AddDrawable(parent);
		Drawable & drawref = GetDrawable(parent, dial_draw);
		drawref.SetTextures(texture->GetId());
		drawref.SetVertArray(&dial_marks);
		drawref.SetCull(false);
		//drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}

	// dial label
	{
		VertexArray temp;
		float w = 0.25 * radius * hwratio;
		float h = 0.25 * radius;
		float angle = startangle;
		float angle_delta = (endangle - startangle) / segments;
		float value = startvalue;
		float value_delta = (endvalue - startvalue) / segments;
		for (int i = 0; i <= segments; ++i)
		{
			std::ostringstream s;
			s << value;
			float x = centerx + 0.75 * sin(angle) * radius * hwratio;
			float y = centery + 0.75 * cos(angle) * radius;
			float xn = TextDraw::RenderText(font, s.str(), x, y, w, h, temp);
			temp.Translate((x - xn) * 0.5, 0, 0);
			dial_label = dial_label + temp;
			angle += angle_delta;
			value += value_delta;
		}

		dialnum_draw = AddTextDrawable(parent);
		Drawable & drawref = GetTextDrawable(parent, dialnum_draw);
		drawref.SetTextures(font.GetFontTexture()->GetId());
		drawref.SetVertArray(&dial_label);
		drawref.SetCull(false);
		//drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}

	// pointer
	{
		float p[] = {-0.015, 0.92, 0, 0.015, 0.92, 0, 0.025, -0.1, 0, -0.025, -0.1, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		unsigned int f[] = {0, 2, 1, 0, 3, 2};
		pointer.Clear();
		pointer.Add(f, 6, p, 12, t, 8);

		pointer_draw = AddDrawable(parent);
		Drawable & drawref = GetDrawable(parent, pointer_draw);
		drawref.SetTextures(texture->GetId());
		drawref.SetVertArray(&pointer_rotated);
		drawref.SetCull(false);
		//drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(2);
	}
}

void HudGauge::Revise(
	SceneNode & parent,
	const Font & font,
	float startvalue,
	float endvalue,
	float valuedelta)
{
	Set(parent, texture, font, scalex / scaley, centerx, centery, scaley,
		startangle, endangle, startvalue, endvalue, valuedelta);
}

void HudGauge::Update(float value)
{
	float angle = value * scale + startangle;
	pointer_rotated = pointer;
	pointer_rotated.Rotate(angle, 0, 0, -1);
	pointer_rotated.Scale(scalex, scaley, 1);
	pointer_rotated.Translate(centerx, centery, 0);
}
