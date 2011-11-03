#include "hudgauge.h"
#include "text_draw.h"

#include <sstream>

inline keyed_container<DRAWABLE>::handle AddDrawable(SCENENODE & node)
{
	return node.GetDrawlist().twodim.insert(DRAWABLE());
}

inline DRAWABLE & GetDrawable(SCENENODE & node, keyed_container <DRAWABLE>::handle & drawhandle)
{
	return node.GetDrawlist().twodim.get(drawhandle);
}

inline void Erase(SCENENODE & node, keyed_container <DRAWABLE>::handle & drawhandle)
{
	if (drawhandle.valid()) node.GetDrawlist().twodim.erase(drawhandle);
}

HUDGAUGE::HUDGAUGE() :
	centerx(0), centery(0), scalex(1), scaley(1), offset(0), scale(1)
{
	// ctor
}

void HUDGAUGE::Set(
	SCENENODE & parent,
	FONT & font,
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
	// calculate number of segments (max 9)
	float segments = (endvalue - startvalue) / valuedelta;
	float factor = ceil(segments / 9.0f);
	segments = ceil(segments / factor);
	valuedelta = valuedelta * factor;
	endvalue = startvalue + segments * valuedelta;

	this->centerx = centerx;
	this->centery = centery;
	this->scalex = radius * hwratio;
	this->scaley = radius;
	this->offset = startangle;
	this->scale = (endangle - startangle) / (endvalue - startvalue);

	// reset
	Erase(parent, pointer_draw);
	Erase(parent, dialnum_draw);
	Erase(parent, dial_draw);
	pointer_rotated.Clear();
	pointer.Clear();
	dialnum.Clear();
	dial.Clear();

	// dial
	{
		// big marker
		float pb[] = {-0.02, 1, 0, 0.02, 1, 0, 0.02, 0.92, 0, -0.02, 0.92, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		int f[] = {0, 2, 1, 0, 3, 2};
		VERTEXARRAY bm;
		bm.SetVertices(pb, 12);
		bm.SetTexCoordSets(1);
		bm.SetTexCoords(0, t, 8);
		bm.SetFaces(f, 6);

		// small marker
		float ps[] = {-0.01, 1, 0, 0.01, 1, 0, 0.01, 0.95, 0, -0.01, 0.95, 0};
		VERTEXARRAY sm;
		sm.SetVertices(ps, 12);
		sm.SetTexCoordSets(1);
		sm.SetTexCoords(0, t, 8);
		sm.SetFaces(f, 6);

		float delta = (endangle - startangle) / (3 * segments);
		float angle = startangle;
		for (int i = 0; i <= 3 * segments; ++i)
		{
			VERTEXARRAY temp = (i % 3) ? sm : bm;
			temp.Rotate(angle, 0, 0, -1);
			dial = dial + temp;
			angle = angle + delta;
		}
		dial.Scale(radius * hwratio, radius, 1);
		dial.Translate(centerx, centery, 0.0);

		dial_draw = AddDrawable(parent);
		DRAWABLE & drawref = GetDrawable(parent, dial_draw);
		drawref.SetVertArray(&dial);
		drawref.SetCull(false, false);
		drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}

	// dial numbers
	{
		float w = 0.25 * radius * hwratio;
		float h = 0.25 * radius;
		float angle = startangle;
		float angle_delta = (endangle - startangle) / segments;
		float value = startvalue;
		float value_delta = (endvalue - startvalue) / segments;
		for (int i = 0; i <= segments; ++i)
		{
			VERTEXARRAY temp;
			std::stringstream sstr;
			std::string text;
			sstr << value;
			sstr >> text;
			float x = centerx + 0.75 * sin(angle) * radius * hwratio;
			float y = centery + 0.75 * cos(angle) * radius;
			float xn = TEXT_DRAW::RenderText(font, text, x, y, w, h, temp);
			temp.Translate((x - xn) * 0.5, 0, 0);
			dialnum = dialnum + temp;
			angle += angle_delta;
			value += value_delta;
		}
		dialnum_draw = AddDrawable(parent);
		DRAWABLE & drawref = GetDrawable(parent, dialnum_draw);
		drawref.SetDiffuseMap(font.GetFontTexture());
		drawref.SetVertArray(&dialnum);
		drawref.SetCull(false, false);
		drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}

	// pointer
	{
		float p[] = {-0.015, 0.92, 0, 0.015, 0.92, 0, 0.025, -0.1, 0, -0.025, -0.1, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		int f[] = {0, 2, 1, 0, 3, 2};
		pointer.SetVertices(p, 12);
		pointer.SetTexCoordSets(1);
		pointer.SetTexCoords(0, t, 8);
		pointer.SetFaces(f, 6);
/*		pointer.Scale(radius, radius, 1);

		pointer_node = parent.AddNode();
		SCENENODE & noderef = parent.GetNode(pointer_node);
		QUATERNION<float> rot(startangle, 0, 0, 1);
		MATHVECTOR<float,3> pos(centerx, centery, 0);
		noderef.GetTransform().SetRotation(rot);
		noderef.GetTransform().SetTranslation(pos);
*/
		pointer_draw = AddDrawable(parent);//noderef);
		DRAWABLE & drawref = GetDrawable(parent, pointer_draw);//noderef, pointer_draw);
		drawref.SetVertArray(&pointer_rotated);//pointer);
		drawref.SetCull(false, false);
		drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}
}

void HUDGAUGE::Update(SCENENODE & parent, float value)
{
	float angle = value * scale + offset;
	pointer_rotated = pointer;
	pointer_rotated.Rotate(angle, 0, 0, -1);
	pointer_rotated.Scale(scalex, scaley, 1);
	pointer_rotated.Translate(centerx, centery, 0);
/*	QUATERNION<float> rot(angle, 1, 0, 0);
	SCENENODE & noderef = parent.GetNode(pointer_node);
	noderef.GetTransform().SetRotation(rot);*/

}
