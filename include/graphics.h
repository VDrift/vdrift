#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <string>
#include <ostream>
#include <map>
#include <list>
#include <vector>

#include "shader.h"
#include "mathvector.h"
#include "fbtexture.h"
#include "scenenode.h"
#include "staticdrawables.h"
#include "matrix4.h"
#include "texture.h"
#include "reseatable_reference.h"
#include "aabb_space_partitioning.h"
#include "glstatemanager.h"
#include "graphics_renderers.h"

#include <SDL/SDL.h>

class SCENENODE;

class GRAPHICS_SDLGL
{
private:
	GLSTATEMANAGER glstate;
	
	int w, h;
	SDL_Surface * surface;
	bool initialized;
	bool using_shaders;
	GLint max_anisotropy;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	float closeshadow;
	enum {REFLECTION_DISABLED, REFLECTION_STATIC, REFLECTION_DYNAMIC} reflection_status;
	TEXTURE static_reflection;
	TEXTURE static_ambient;
	
	std::map <std::string, SHADER_GLSL> shadermap;
	std::map <std::string, SHADER_GLSL>::iterator activeshader;
	
	DRAWABLE_CONTAINER <PTRVECTOR> dynamic_drawlist; //used for objects that move or change
	STATICDRAWABLES static_drawlist; //used for objects that will never change
	
	//render pipeline info
	RENDER_INPUT_SCENE renderscene;
	//RENDER_OUTPUT scene_depthtexture;
	std::list <RENDER_OUTPUT> shadow_depthtexturelist;
	RENDER_OUTPUT final;
	RENDER_OUTPUT edgecontrastenhancement_depths;
	RENDER_OUTPUT full_scene_buffer;
	RENDER_OUTPUT bloom_buffer;
	RENDER_OUTPUT blur_buffer;
	RENDER_OUTPUT dynamic_reflection;
	
	float camfov;
	float view_distance;
	MATHVECTOR <float, 3> campos;
	QUATERNION <float> camorient;
	QUATERNION <float> lightdirection;
	MATHVECTOR <float, 3> dynamic_reflection_sample_position;
	
	unsigned int fsaa;
	int lighting; ///<lighting quality; see data/settings/options.config for definition of values
	bool bloom;
	float contrast;
	bool aticard;
	
	void ChangeDisplay(const int width, const int height, const int bpp, const int dbpp, const bool fullscreen, 
			   unsigned int antialiasing, std::ostream & info_output, std::ostream & error_output);
	void SetActiveShader(const std::string name);
	bool LoadShader(const std::string & shaderpath, const std::string & name, std::ostream & info_output, std::ostream & error_output, std::string variant="", std::string variant_defines="");
	void EnableShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output);
	void DisableShaders();
	void DrawBox(const MATHVECTOR <float, 3> & corner1, const MATHVECTOR <float, 3> & corner2) const;
	void RenderDrawlist(std::vector <DRAWABLE*> & drawlist,
						RENDER_INPUT_SCENE & render_scene, 
						RENDER_OUTPUT & render_output, 
						std::ostream & error_output);
	void RenderDrawlists(std::vector <DRAWABLE*> & dynamic_drawlist,
						std::vector <DRAWABLE*> & static_drawlist,
						RENDER_INPUT_SCENE & render_scene, 
						RENDER_OUTPUT & render_output, 
						std::ostream & error_output);
	void RenderPostProcess(const std::string & shadername,
						FBTEXTURE & texture0, 
						RENDER_OUTPUT & render_output, 
						std::ostream & error_output);
	
	void Render(RENDER_INPUT * input, RENDER_OUTPUT & output, std::ostream & error_output);
public:
	GRAPHICS_SDLGL() : surface(NULL),initialized(false),using_shaders(false),max_anisotropy(0),shadows(false),
		       	closeshadow(5.0), reflection_status(REFLECTION_DISABLED),camfov(45),
			view_distance(10000),fsaa(1),lighting(0),bloom(false),contrast(1.0), aticard(false)
			{activeshader = shadermap.end();}
	~GRAPHICS_SDLGL() {}
	
	typedef DRAWABLE_CONTAINER <PTRVECTOR> dynamicdrawlist_type;
	
	///reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	void Init(const std::string shaderpath, const std::string & windowcaption,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen, bool shaders,
				unsigned int antialiasing, bool enableshadows,
				int shadow_distance, int shadow_quality,
				int reflection_type,
				const std::string & static_reflectionmap_file,
				const std::string & static_ambientmap_file,
				int anisotropy, const std::string & texturesize,
				int lighting_quality, bool newbloom,
				std::ostream & info_output, std::ostream & error_output);
	void Deinit();
	void BeginScene(std::ostream & error_output);
	DRAWABLE_CONTAINER <PTRVECTOR> & GetDynamicDrawlist() {return dynamic_drawlist;}
	void AddStaticNode(SCENENODE & node, bool clearcurrent = true);
	void SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
					const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos)
	{
		camfov = fov;
		campos = cam_position;
		camorient = cam_rotation;
		view_distance = new_view_distance;
		dynamic_reflection_sample_position = dynamic_reflection_sample_pos;
	}
	void DrawScene(std::ostream & error_output);
	void EndScene(std::ostream & error_output);
	void Screenshot(std::string filename);
	int GetW() const {return w;}
	int GetH() const {return h;}
	float GetWHRatio() const {return (float)w/h;}
	int GetMaxAnisotropy() const {return max_anisotropy;}
	bool AntialiasingSupported() {return GLEW_ARB_multisample;}

	bool GetUsingShaders() const
	{
		return using_shaders;
	}
	
	bool ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output);

	void SetCloseShadow ( float value )
	{
		closeshadow = value;
	}

	bool GetShadows() const
	{
		return shadows;
	}

	void SetSunDirection ( const QUATERNION< float >& value )
	{
		lightdirection = value;
	}

	void SetContrast ( float value )
	{
		contrast = value;
	}
	
	bool HaveATICard() {return aticard;}
};

#endif

