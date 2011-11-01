#include "hudgauge.h"

inline keyed_container<DRAWABLE>::handle AddDrawable(SCENENODE & node)
{
	return node.GetDrawlist().twodim.insert(DRAWABLE());
}

inline DRAWABLE & GetDrawable(SCENENODE & node, keyed_container <DRAWABLE>::handle & drawhandle)
{
	return node.GetDrawlist().twodim.get(drawhandle);
}

HUDGAUGE::HUDGAUGE() : offset(0), scale(1)
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
	int startvalue,
	int endvalue,
	int numvalues,
	std::ostream & error_output)
{
	offset = startangle;
	scale = (endangle - startangle) / numvalues;

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

		float delta = (endangle - startangle) / (3 * numvalues);
		float angle = startangle;
		for (int i = 0; i <= 3 * numvalues; ++i)
		{
			VERTEXARRAY temp = (i % 3) ? sm : bm;
			temp.Rotate(angle, 0, 0, 1);
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

	// pointer
	{
		VERTEXARRAY ptr;
		float pp[] = {-0.01, 0.92, 0, 0.01, 0.92, 0, 0.025, -0.1, 0, -0.025, -0.1, 0};
		float t[] = {0, 0, 1, 0, 1, 1, 0, 1};
		int f[] = {0, 2, 1, 0, 3, 2};
		ptr.SetVertices(pp, 12);
		ptr.SetTexCoordSets(1);
		ptr.SetTexCoords(0, t, 8);
		ptr.SetFaces(f, 6);
		ptr.Scale(radius, radius, 1);

		pointer_node = parent.AddNode();
		SCENENODE & noderef = parent.GetNode(pointer_node);
		QUATERNION<float> rot(startangle, 0, 0, 1);
		MATHVECTOR<float,3> pos(centerx, centery, 0);
		noderef.GetTransform().SetRotation(rot);
		noderef.GetTransform().SetTranslation(pos);

		pointer_draw = AddDrawable(noderef);
		DRAWABLE & drawref = GetDrawable(noderef, pointer_draw);
		drawref.SetVertArray(&ptr);
		drawref.SetCull(false, false);
		drawref.SetColor(1, 1, 1, 0.5);
		drawref.SetDrawOrder(1);
	}
}

void HUDGAUGE::Update(SCENENODE & parent, float value)
{
	SCENENODE & noderef = parent.GetNode(pointer_node);
	QUATERNION<float> rot(value * scale + offset, 0, 0, 1);
	noderef.GetTransform().SetRotation(rot);
}
