#ifndef _RENDER_INPUT_SCENE_H
#define _RENDER_INPUT_SCENE_H

#include "render_input.h"
#include "mathvector.h"
#include "quaternion.h"
#include "matrix4.h"
#include "frustum.h"
#include "reseatable_reference.h"
#include <vector>

class SCENENODE;
class DRAWABLE;
class TEXTURE_INTERFACE;
class SHADER_GLSL;

class RENDER_INPUT_SCENE : public RENDER_INPUT
{
public:
	RENDER_INPUT_SCENE();

	~RENDER_INPUT_SCENE();

	void SetDrawLists(
		std::vector <DRAWABLE*> & dl_dynamic,
		std::vector <DRAWABLE*> & dl_static);

	void DisableOrtho();

	void SetOrtho(
		const MATHVECTOR <float, 3> & neworthomin,
		const MATHVECTOR <float, 3> & neworthomax);

	FRUSTUM SetCameraInfo(
		const MATHVECTOR <float, 3> & newpos,
		const QUATERNION <float> & newrot,
		float newfov, float newlodfar,
		float neww, float newh,
		bool restore_matrices = true);

	void SetSunDirection(const MATHVECTOR <float, 3> & newsun);

	void SetFlags(bool newshaders);

	void SetDefaultShader(SHADER_GLSL & newdefault);

	void SetClear(bool newclearcolor, bool newcleardepth);

	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output);

	void SetReflection(TEXTURE_INTERFACE * value);

	void SetFSAA(unsigned int value);

	void SetAmbient(TEXTURE_INTERFACE & value);

	void SetContrast(float value);

	void SetDepthMode(int mode);

	void SetWriteDepth(bool write);

	void SetWriteColor(bool write);

	void SetWriteAlpha(bool write);

	std::pair <bool, bool> GetClear() const;

	void SetCarPaintHack(bool hack);

	void SetBlendMode(BLENDMODE::BLENDMODE mode);

private:
	reseatable_reference <std::vector <DRAWABLE*> > dynamic_drawlist_ptr;
	reseatable_reference <std::vector <DRAWABLE*> > static_drawlist_ptr;
	bool last_transform_valid;
	MATRIX4 <float> last_transform;
	QUATERNION <float> cam_rotation; //used for the skybox effect
	MATHVECTOR <float, 3> cam_position;
	MATHVECTOR <float, 3> lightposition;
	MATHVECTOR <float, 3> orthomin;
	MATHVECTOR <float, 3> orthomax;
	float w, h;
	float camfov;
	FRUSTUM frustum; //used for frustum culling
	float lod_far; //used for distance culling
	bool shaders;
	bool clearcolor, cleardepth;
	reseatable_reference <SHADER_GLSL> shader;
	reseatable_reference <TEXTURE_INTERFACE> reflection;
	reseatable_reference <TEXTURE_INTERFACE> ambient;
	bool orthomode;
	unsigned int fsaa;
	float contrast;
	int depth_mode;
	bool writecolor;
	bool writealpha;
	bool writedepth;
	bool carpainthack;
	BLENDMODE::BLENDMODE blendmode;

	void DrawList(GLSTATEMANAGER & glstate, std::vector <DRAWABLE*> & drawlist, bool preculled);

	/// returns true if the object was culled and should not be drawn
	bool FrustumCull(DRAWABLE & tocull);

	void SelectAppropriateShader(DRAWABLE & forme);

	void SelectFlags(DRAWABLE & forme, GLSTATEMANAGER & glstate);

	void SelectTexturing(DRAWABLE & forme, GLSTATEMANAGER & glstate);

	bool SelectTransformStart(DRAWABLE & forme, GLSTATEMANAGER & glstate);

	void SelectTransformEnd(DRAWABLE & forme, bool need_pop);
};

#endif // _RENDER_INPUT_SCENE_H
