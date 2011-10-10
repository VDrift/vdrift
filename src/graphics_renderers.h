#ifndef _GRAPHICS_RENDERERS_H
#define _GRAPHICS_RENDERERS_H

#include "shader.h"
#include "mathvector.h"
#include "quaternion.h"
#include "fbobject.h"
#include "matrix4.h"
#include "reseatable_reference.h"
#include "glstatemanager.h"
#include "frustum.h"

#include <SDL/SDL.h>
#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

class SCENENODE;
class DRAWABLE;
class TEXTURE_INTERFACE;

namespace BLENDMODE
{
enum BLENDMODE
{
	DISABLED,
	ADD,
	ALPHABLEND,
	PREMULTIPLIED_ALPHA,
	ALPHATEST
};
}

///purely abstract base class
class RENDER_INPUT
{
public:
	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output) = 0; ///< must be implemented by derived classes
};

class RENDER_INPUT_POSTPROCESS : public RENDER_INPUT
{
public:
	RENDER_INPUT_POSTPROCESS() :
		shader(NULL),
		writealpha(true),
		writecolor(true),
		writedepth(false),
		depth_mode(GL_LEQUAL),
		clearcolor(false),
		cleardepth(false),
		blendmode(BLENDMODE::DISABLED),
		contrast(1.0)
		{}

	void SetSourceTextures(const std::vector <TEXTURE_INTERFACE*> & textures)
	{
		source_textures = textures;
	}

	void SetShader(SHADER_GLSL * newshader) {shader = newshader;}
	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output);
	void SetWriteColor(bool write) {writecolor = write;}
	void SetWriteAlpha(bool write) {writealpha = write;}
	void SetWriteDepth(bool write) {writedepth = write;}
	void SetDepthMode ( int mode ) {depth_mode = mode;}
	void SetClear(bool newclearcolor, bool newcleardepth) {clearcolor = newclearcolor;cleardepth = newcleardepth;}
	void SetBlendMode(BLENDMODE::BLENDMODE mode) {blendmode = mode;}
	void SetContrast ( float value ) {contrast = value;}

	// these are used only to upload uniforms to the shaders
	void SetCameraInfo(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newrot, float newfov, float newlodfar, float neww, float newh);
	void SetSunDirection(const MATHVECTOR <float, 3> & newsun) {lightposition = newsun;}

private:
	std::vector <TEXTURE_INTERFACE*> source_textures;
	SHADER_GLSL * shader;
	bool writealpha;
	bool writecolor;
	bool writedepth;
	MATHVECTOR <float, 3> lightposition;
	QUATERNION <float> cam_rotation;
	MATHVECTOR <float, 3> cam_position;
	float w, h;
	float camfov;
	float lod_far;
	FRUSTUM frustum;
	int depth_mode;
	bool clearcolor, cleardepth;
	BLENDMODE::BLENDMODE blendmode;
	float contrast;
};

class RENDER_INPUT_SCENE : public RENDER_INPUT
{
public:
	RENDER_INPUT_SCENE();

	void SetDrawLists(std::vector <DRAWABLE*> & dl_dynamic, std::vector <DRAWABLE*> & dl_static)
	{
		dynamic_drawlist_ptr = &dl_dynamic;
		static_drawlist_ptr = &dl_static;
	}
	void DisableOrtho() {orthomode = false;}
	void SetOrtho(const MATHVECTOR <float, 3> & neworthomin, const MATHVECTOR <float, 3> & neworthomax) {orthomode = true; orthomin = neworthomin; orthomax = neworthomax;}
	FRUSTUM SetCameraInfo(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newrot, float newfov, float newlodfar, float neww, float newh, bool restore_matrices = true);
	void SetSunDirection(const MATHVECTOR <float, 3> & newsun) {lightposition = newsun;}
	void SetFlags(bool newshaders) {shaders=newshaders;}
	void SetDefaultShader(SHADER_GLSL & newdefault) {assert(newdefault.GetLoaded());shader = newdefault;}
	void SetClear(bool newclearcolor, bool newcleardepth) {clearcolor = newclearcolor;cleardepth = newcleardepth;}
	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output);
	void SetReflection ( TEXTURE_INTERFACE * value ) {if (!value) reflection.clear(); else reflection = value;}
	void SetFSAA ( unsigned int value ) {fsaa = value;}
	void SetAmbient ( TEXTURE_INTERFACE & value ) {ambient = value;}
	void SetContrast ( float value ) {contrast = value;}
	void SetDepthMode ( int mode ) {depth_mode = mode;}
	void SetWriteDepth(bool write) {writedepth = write;}
	void SetWriteColor(bool write) {writecolor = write;}
	void SetWriteAlpha(bool write) {writealpha = write;}
	std::pair <bool, bool> GetClear() const {return std::make_pair(clearcolor, cleardepth);}
	void SetCarPaintHack(bool hack) {carpainthack = hack;}
	void SetBlendMode(BLENDMODE::BLENDMODE mode) {blendmode = mode;}

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
	bool FrustumCull(DRAWABLE & tocull);
	void SelectAppropriateShader(DRAWABLE & forme);
	void SelectFlags(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	void SelectTexturing(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	bool SelectTransformStart(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	void SelectTransformEnd(DRAWABLE & forme, bool need_pop);
};

class RENDER_OUTPUT
{
public:
	RENDER_OUTPUT() : target(RENDER_TO_FRAMEBUFFER) {}

	///returns the FBO that the user should set up as necessary
	FBOBJECT & RenderToFBO()
	{
		target = RENDER_TO_FBO;
		return fbo;
	}

	void RenderToFramebuffer()
	{
		target = RENDER_TO_FRAMEBUFFER;
	}

	bool IsFBO() const {return target == RENDER_TO_FBO;}

	void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output)
	{
		if (target == RENDER_TO_FBO)
			fbo.Begin(glstate, error_output);
		else if (target == RENDER_TO_FRAMEBUFFER)
			glstate.BindFramebuffer(0);
	}

	void End(GLSTATEMANAGER & glstate, std::ostream & error_output)
	{
		if (target == RENDER_TO_FBO)
			fbo.End(glstate, error_output);
	}

private:
	FBOBJECT fbo;
	enum
	{
		RENDER_TO_FBO,
		RENDER_TO_FRAMEBUFFER
	} target;
};

#endif

