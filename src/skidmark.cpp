#include "skidmark.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include <cassert>

static keyed_container<DRAWABLE> & GetDrawList(SCENENODE & node)
{
	return node.GetDrawlist().normal_blend;
}

static void FadeOut(float alpha_delta, DRAWABLE & drawable)
{
	float alpha = drawable.GetAlpha() - alpha_delta;
	if (alpha < 0)
	{
		alpha = 0;
		drawable.SetDrawEnable(false);
	}
	drawable.SetAlpha(alpha);
}

SKIDMARK::STRIP::STRIP() :
	hwidth(0),
	length(0),
	strength(0)
{
	// ctor
}

SKIDMARK::SKIDMARK(
	int strip_count,
	int strip_length,
	float strength_min,
	float length_min,
	float offset) :
	strip_count(strip_count),
	strip_length(strip_length),
	strength_min(strength_min),
	length_min(length_min),
	offset(offset),
	strip(strip_count),
	strip_next(0),
	strip_old(0)
{
	// ctor
}

bool SKIDMARK::Init(
	ContentManager & content,
	const std::string & path,
	const std::string & texname)
{
	TEXTUREINFO info;
	info.repeatu = false;
	info.repeatv = false;
	std::tr1::shared_ptr<TEXTURE> tex;
	if (!content.load(path, texname, info, tex)) return false;

	keyed_container<DRAWABLE> & drawlist = GetDrawList(node);
	for (unsigned int i = 0; i < strip_count; ++i)
	{
		strip[i].draw = drawlist.insert(DRAWABLE());
		DRAWABLE & drawable = drawlist.get(strip[i].draw);
		drawable.SetDiffuseMap(tex);
		drawable.SetVertArray(&strip[i].varray);
		drawable.SetDecal(true);
	}

	return true;
}

void SKIDMARK::Reset()
{
	keyed_container<DRAWABLE> & drawlist = GetDrawList(node);
	for (unsigned int i = 0; i < strip_count; ++i)
	{
		drawlist.get(strip[i].draw).SetDrawEnable(false);
		drawlist.get(strip[i].draw).SetAlpha(1.0);
		strip[i].varray.Clear();
		strip[i].hwidth = 0;
		strip[i].strength = 0;
		strip[i].length = 0;
	}
}

void SKIDMARK::Update(
	unsigned int & id,
	const MATHVECTOR<float, 3> & position,
	const MATHVECTOR<float, 3> & left,
	const MATHVECTOR<float, 3> & normal,
	const float half_width,
	const float strength)
{
	// strength too low to create new strip
	bool new_strip = id < 0 || id >= strip_count;
	if (new_strip && strength < strength_min)
	{
		return;
	}

	// start a new strip, continue old
	if (new_strip || strip[id].length == strip_length)
	{
		unsigned int id_old = id;
		id = strip_next;

		strip_next++;
		if (strip_next >= strip_count) strip_next = 0;

		strip[id].varray.Clear();
		strip[id].length = 0;
		if (new_strip)
		{
			//std::clog << "start new skidmark: " << id << std::endl;
			strip[id].position = position + normal * offset;
			strip[id].left = left;
			strip[id].normal = normal;
			strip[id].hwidth = half_width;
			strip[id].strength = strength;
			return;
		}
		//std::clog << "continue skidmark id: " << id_old << " new id: " << id << std::endl;
		strip[id].position = strip[id_old].position;
		strip[id].left = strip[id_old].left;
		strip[id].normal = strip[id_old].normal;
		strip[id].hwidth = strip[id_old].hwidth;
		strip[id].strength = 0.5 * (strip[id_old].strength + strength);
	}

	STRIP & s = strip[id];
	s.strength = 0.5 * (s.strength + strength);

	// check distance to the previous strip point
	if ((position - strip[id].position).MagnitudeSquared() < length_min)
	{
		return;
	}

	// add a quad
	MATHVECTOR<float, 3> c[4];
	c[0] = s.position - s.left * s.hwidth;
	c[1] = s.position + s.left * s.hwidth;
	s.position = position + normal * offset;
	s.left = left;
	c[2] = s.position - s.left * s.hwidth;
	c[3] = s.position + s.left * s.hwidth;

	const int vcount = 3 * 4;
	const float v[vcount] = {
		c[0][0], c[0][1], c[0][2], c[1][0], c[1][1], c[1][2],
		c[2][0], c[2][1], c[2][2], c[3][0], c[3][1], c[3][2]};

	const int ncount = 3 * 4;
	const float n[ncount] = {
		s.normal[0], s.normal[1], s.normal[2], s.normal[0], s.normal[1], s.normal[2],
		normal[0], normal[1], normal[2], normal[0], normal[1], normal[2]};

	const int fcount = 3 * 2;
	const int f[fcount] = {0, 1, 2, 2, 1, 3};


	const int tcount = 2 * 4;
	float t[tcount] = {
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.5, 1.0, 0.5};

	// finish stip if strength below 50% min strength(hyseresis)
	if (s.strength < 0.5 * strength_min)
	{
		//std::clog << "finish skid: " << id << " len: " << s.length << std::endl;
		t[1] = t[3] = 0.5;
		t[5] = t[7] = 1.0;
		id = -1;
	}

	s.varray.Add(n, ncount, v, vcount, f, fcount, t, tcount);
	s.normal = normal;
	s.length++;
}

void SKIDMARK::Update()
{
	int created = strip_next - strip_old;
	if (created == 0) return;

	// fade out skidmarks
	keyed_container<DRAWABLE> & drawlist = GetDrawList(node);
	float alpha_delta = created / (float)strip_count;
	if (alpha_delta < 0)
	{
		alpha_delta = -alpha_delta;
		for (unsigned int i = 0; i < strip_next; ++i)
		{
			drawlist.get(strip[i].draw).SetAlpha(1.0);
			drawlist.get(strip[i].draw).SetDrawEnable(true);
		}
		for (unsigned int i = strip_next; i < strip_old; ++i)
		{
			FadeOut(alpha_delta, drawlist.get(strip[i].draw));
		}
		for (unsigned int i = strip_old; i < strip_count; ++i)
		{
			drawlist.get(strip[i].draw).SetAlpha(1.0);
			drawlist.get(strip[i].draw).SetDrawEnable(true);
		}
	}
	else
	{
		for (unsigned int i = 0; i < strip_old; ++i)
		{
			FadeOut(alpha_delta, drawlist.get(strip[i].draw));
		}
		for (unsigned int i = strip_old; i < strip_next; ++i)
		{
			drawlist.get(strip[i].draw).SetAlpha(1.0);
			drawlist.get(strip[i].draw).SetDrawEnable(true);
		}
		for (unsigned int i = strip_next; i < strip_count; ++i)
		{
			FadeOut(alpha_delta, drawlist.get(strip[i].draw));
		}
	}

	strip_old = strip_next;
}
