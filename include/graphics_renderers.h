#ifndef _GRAPHICS_RENDERERS_H
#define _GRAPHICS_RENDERERS_H

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

#include "shader.h"
#include "mathvector.h"
#include "quaternion.h"
#include "fbtexture.h"
#include "matrix4.h"
#include "texture.h"
#include "reseatable_reference.h"
#include "aabb_space_partitioning.h"
#include "glstatemanager.h"
#include "frustum.h"

#include <SDL/SDL.h>

class SCENENODE;
class DRAWABLE;

///purely abstract base class
class RENDER_INPUT
{
public:
	virtual void Render(GLSTATEMANAGER & glstate) = 0; ///< must be implemented by derived classes
};

class RENDER_INPUT_POSTPROCESS : public RENDER_INPUT
{
public:
	RENDER_INPUT_POSTPROCESS() : source_texture(NULL), shader(NULL) {}
	
	void SetSourceTexture(FBTEXTURE & newsource)
	{
		source_texture = &newsource;
	}
	
	void SetShader(SHADER_GLSL * newshader) {shader = newshader;}
	virtual void Render(GLSTATEMANAGER & glstate);
	
private:
	const FBTEXTURE * source_texture;
	SHADER_GLSL * shader;
};

class RENDER_INPUT_SCENE : public RENDER_INPUT
{
public:
	enum SHADER_TYPE
	{
		SHADER_SIMPLE,
		SHADER_DISTANCEFIELD,
		SHADER_FULL,
		SHADER_FULLBLEND,
		SHADER_SKYBOX,
		SHADER_NONE
	};
	
	RENDER_INPUT_SCENE();
	
	void SetDrawLists(std::vector <DRAWABLE*> & dl_dynamic, std::vector <DRAWABLE*> & dl_static)
	{
		dynamic_drawlist_ptr = &dl_dynamic;
		static_drawlist_ptr = &dl_static;
	}
	void DisableOrtho() {orthomode = false;}
	void SetOrtho(const MATHVECTOR <float, 3> & neworthomin, const MATHVECTOR <float, 3> & neworthomax) {orthomode = true; orthomin = neworthomin; orthomax = neworthomax;}
	FRUSTUM SetCameraInfo(const MATHVECTOR <float, 3> & newpos, const QUATERNION <float> & newrot, float newfov, float newlodfar, float neww, float newh);
	void SetSunDirection(const MATHVECTOR <float, 3> & newsun) {lightposition = newsun;}
	void SetFlags(bool newshaders) {shaders=newshaders;}
	void SetDefaultShader(SHADER_GLSL & newdefault) {assert(newdefault.GetLoaded());shadermap.clear();shadermap.resize(SHADER_NONE, &newdefault);}
	void SetShader(SHADER_TYPE stype, SHADER_GLSL & newshader) {assert((unsigned int)stype < shadermap.size());shadermap[stype]=&newshader;}
	void SetClear(bool newclearcolor, bool newcleardepth) {clearcolor = newclearcolor;cleardepth = newcleardepth;}
	virtual void Render(GLSTATEMANAGER & glstate);
	void SetReflection ( TEXTURE_INTERFACE * value ) {if (!value) reflection.clear(); else reflection = value;}
	void SetFSAA ( unsigned int value ) {fsaa = value;}
	void SetAmbient ( TEXTURE_INTERFACE & value ) {ambient = value;}
	void SetContrast ( float value ) {contrast = value;}
	void SetDepthModeEqual ( bool value ) {depth_mode_equal = value;}
	void SetWriteDepth(bool write) {writedepth = write;}
	void SetWriteColor(bool write) {writecolor = write;}
	std::pair <bool, bool> GetClear() const {return std::make_pair(clearcolor, cleardepth);}

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
	std::vector <SHADER_GLSL *> shadermap;
	SHADER_TYPE activeshader;
	reseatable_reference <TEXTURE_INTERFACE> reflection;
	reseatable_reference <TEXTURE_INTERFACE> ambient;
	bool orthomode;
	unsigned int fsaa;
	float contrast;
	bool depth_mode_equal;
	bool writecolor;
	bool writedepth;
	
	void DrawList(GLSTATEMANAGER & glstate, std::vector <DRAWABLE*> & drawlist, bool preculled);
	bool FrustumCull(DRAWABLE & tocull);
	void SelectAppropriateShader(DRAWABLE & forme);
	void SelectFlags(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	void SelectTexturing(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	bool SelectTransformStart(DRAWABLE & forme, GLSTATEMANAGER & glstate);
	void SelectTransformEnd(DRAWABLE & forme, bool need_pop);
	void ExtractFrustum();
	//unsigned int CombineDrawlists(); ///< returns the number of scenedraw elements that have already gone through culling
	void SetActiveShader(const SHADER_TYPE & newshader);
};

class RENDER_OUTPUT
{
public:
	RENDER_OUTPUT() : target(RENDER_TO_FRAMEBUFFER) {}
	
	///returns the FBO that the user should set up as necessary
	FBTEXTURE & RenderToFBO()
	{
		target = RENDER_TO_FBO;
		return fbo;
	}
	
	void RenderToFramebuffer()
	{
		target = RENDER_TO_FRAMEBUFFER;
	}
	
	void Begin(GLSTATEMANAGER & glstate, std::ostream & error_output)
	{
		if (target == RENDER_TO_FBO)
			fbo.Begin(glstate, error_output);
		else if (target == RENDER_TO_FRAMEBUFFER)
			glstate.BindFramebuffer(0);
	}
	
	void End(std::ostream & error_output)
	{
		if (target == RENDER_TO_FBO)
			fbo.End(error_output);
	}
	
private:
	FBTEXTURE fbo;
	enum
	{
		RENDER_TO_FBO,
		RENDER_TO_FRAMEBUFFER
	} target;
};

#endif

