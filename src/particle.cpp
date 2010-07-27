#include "particle.h"

Particle::Particle(
	SCENENODE & parentnode,
	const MATHVECTOR <float, 3> & new_start_position,
	const MATHVECTOR <float, 3> & new_dir,
	float newspeed,
	float newtrans,
	float newlong,
	float newsize,
	TexturePtr texture) :
	start_position(new_start_position),
	direction(new_dir),
	speed(newspeed),
	transparency(newtrans),
	longevity(newlong),
	size(newsize),
	time(0)
{
	node = parentnode.AddNode();
	SCENENODE & noderef = parentnode.GetNode(node);
	//std::cout << "Created node: " << &node.get() << endl;
	draw = GetDrawlist(noderef).insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawlist(noderef).get(draw);
	drawref.SetDrawEnable(false);
	drawref.SetVertArray(&varray);
	drawref.SetDiffuseMap(texture);
	drawref.SetCull(false,false);
}

float lerp(float x, float y, float s)
{
	float sclamp = std::max(0.f,std::min(1.0f,s));
	return x + sclamp*(y-x);
}

void Particle::Update(
	SCENENODE & parent,
	float dt,
	const QUATERNION <float> & camdir_conjugate,
	const MATHVECTOR <float, 3> & campos)
{
	time += dt;

	SCENENODE & noderef = parent.GetNode(node);
	DRAWABLE & drawref = GetDrawlist(noderef).get(draw);
	drawref.SetVertArray(&varray);

	MATHVECTOR <float, 3> curpos = start_position + direction * time * speed;
	noderef.GetTransform().SetTranslation(curpos);
	noderef.GetTransform().SetRotation(camdir_conjugate);

	float sizescale = 1.0;
	float trans = transparency*std::pow((double)(1.0-time/longevity),4.0);
	float transmax = 1.0;
	if (trans > transmax)
		trans = transmax;
	if (trans < 0.0)
		trans = 0.0;

	sizescale = 5.0*(time/longevity)+1.0;

	varray.SetToBillboard(-sizescale,-sizescale,sizescale,sizescale);
	drawref.SetRadius(sizescale);

	bool drawenable = true;

	// scale the alpha by the closeness to the camera
	// if we get too close, don't draw
	// this prevents major slowdown when there are a lot of particles right next to the camera
	float camdist = (curpos - campos).Magnitude();
	//std::cout << camdist << std::endl;
	const float camdist_off = 3.0;
	const float camdist_full = 4.0;
	trans = lerp(0.f,trans,(camdist-camdist_off)/(camdist_full-camdist_off));
	if (trans <= 0)
		drawenable = false;

	drawref.SetColor(1,1,1,trans);
	drawref.SetDrawEnable(drawenable);
}
