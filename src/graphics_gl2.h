#ifndef _GRAPHICS_GL2_H
#define _GRAPHICS_GL2_H

#include "graphics.h"
#include "graphics_config.h"
#include "glstatemanager.h"
#include "texture.h"
#include "staticdrawables.h"
#include "render_input_postprocess.h"
#include "render_input_scene.h"
#include "render_output.h"

struct GRAPHICS_CAMERA;
class SHADER_GLSL;
class SCENENODE;

class GRAPHICS_GL2 : public GRAPHICS
{
public:
	GRAPHICS_GL2();

	~GRAPHICS_GL2();

	/// reflection_type is 0 (low=OFF), 1 (medium=static), 2 (high=dynamic)
	/// returns true on success
	virtual bool Init(
		const std::string & shaderpath,
		unsigned int resx, unsigned int resy, unsigned int bpp,
		unsigned int depthbpp, bool fullscreen, bool shaders,
		unsigned int antialiasing, bool enableshadows,
		int shadow_distance, int shadow_quality,
		int reflection_type,
		const std::string & static_reflectionmap_file,
		const std::string & static_ambientmap_file,
		int anisotropy, int texturesize,
		int lighting_quality, bool newbloom, bool newnormalmaps,
		const std::string & renderconfig,
		std::ostream & info_output, std::ostream & error_output);

	virtual void Deinit();

	virtual void BeginScene(std::ostream & error_output);

	virtual DRAWABLE_CONTAINER <PTRVECTOR> & GetDynamicDrawlist();

	virtual void AddStaticNode(SCENENODE & node, bool clearcurrent = true);

	/// Setup scene cameras
	virtual void SetupScene(
		float fov, float new_view_distance,
		const MATHVECTOR <float, 3> cam_position,
		const QUATERNION <float> & cam_rotation,
		const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos);

	virtual void DrawScene(std::ostream & error_output);

	virtual void EndScene(std::ostream & error_output);

	virtual int GetMaxAnisotropy() const;

	virtual bool AntialiasingSupported() const;

	virtual bool GetUsingShaders() const;

	virtual bool ReloadShaders(
		const std::string & shaderpath,
		std::ostream & info_output,
		std::ostream & error_output);

	virtual void SetCloseShadow(float value);

	virtual bool GetShadows() const;

	virtual void SetSunDirection(const QUATERNION< float > & value);

	virtual void SetContrast(float value);

private:
	// avoids sending excessive state changes to OpenGL
	GLSTATEMANAGER glstate;

	// configuration variables, internal data
	int w, h;
	bool initialized;
	bool using_shaders;
	GLint max_anisotropy;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	float closeshadow;
	unsigned int fsaa;
	int lighting; ///<lighting quality; see data/settings/options.config for definition of values
	bool bloom;
	bool normalmaps;
	float contrast;
	enum {REFLECTION_DISABLED, REFLECTION_STATIC, REFLECTION_DYNAMIC} reflection_status;
	TEXTURE static_reflection;
	TEXTURE static_ambient;
	std::string renderconfigfile;

	// configuration variables in a data-driven friendly format
	std::set <std::string> conditions;
	GRAPHICS_CONFIG config;

	// shaders
	typedef std::map <std::string, SHADER_GLSL> shader_map_type;
	shader_map_type shadermap;
	shader_map_type::iterator activeshader;

	// scenegraph output
	DRAWABLE_CONTAINER <PTRVECTOR> dynamic_drawlist; //used for objects that move or change
	STATICDRAWABLES static_drawlist; //used for objects that will never change

	// render outputs
	typedef std::map <std::string, RENDER_OUTPUT> render_output_map_type;
	render_output_map_type render_outputs;
	typedef std::map <std::string, FBTEXTURE> texture_output_map_type;
	texture_output_map_type texture_outputs;

	// outputs and other textures used as inputs
	std::map <std::string, reseatable_reference <TEXTURE_INTERFACE> > texture_inputs;

	// render input objects
	RENDER_INPUT_SCENE renderscene;
	RENDER_INPUT_POSTPROCESS postprocess;

	// camera data
	typedef std::map <std::string, GRAPHICS_CAMERA> camera_map_type;
	camera_map_type cameras;

	QUATERNION <float> lightdirection;

	void ChangeDisplay(
		const int width, const int height,
		const int bpp, const int dbpp,
		const bool fullscreen,
		unsigned int antialiasing,
		std::ostream & info_output,
		std::ostream & error_output);

	/// note that if variant is passed in, it is used as the shader name and the shader is also loaded with the variant_defines set
	/// this allows loading multiple shaders from the same shader file, just with different defines set
	/// variant_defines is a space delimited list of defines
	bool LoadShader(
		const std::string & shaderpath,
		const std::string & name,
		std::ostream & info_output,
		std::ostream & error_output,
		std::string variant="",
		std::string variant_defines="");

	void EnableShaders(
		const std::string & shaderpath,
		std::ostream & info_output,
		std::ostream & error_output);

	void DisableShaders(
		const std::string & shaderpath,
		std::ostream & error_output);

	void CullScenePass(
		const GRAPHICS_CONFIG_PASS & pass,
		std::map <std::string, PTRVECTOR <DRAWABLE> > & culled_static_drawlist,
		std::ostream & error_output);

	void DrawScenePass(
		const GRAPHICS_CONFIG_PASS & pass,
		std::map <std::string, PTRVECTOR <DRAWABLE> > & culled_static_drawlist,
		std::ostream & error_output);

	/// draw postprocess scene pass
	void DrawScenePassPost(
		const GRAPHICS_CONFIG_PASS & pass,
		std::map <std::string, PTRVECTOR <DRAWABLE> > & culled_static_drawlist,
		std::ostream & error_output);

	/// get input textures vector from config inputs
	void GetScenePassInputTextures(
		const GRAPHICS_CONFIG_INPUTS & inputs,
		std::vector <TEXTURE_INTERFACE*> & input_textures);

	void DrawScenePassLayer(
		const std::string & layer,
		const GRAPHICS_CONFIG_PASS & pass,
		const std::vector <TEXTURE_INTERFACE*> & input_textures,
		const std::map <std::string, PTRVECTOR <DRAWABLE> > & culled_static_drawlist,
		RENDER_OUTPUT & render_output,
		std::ostream & error_output);

	void RenderDrawlist(
		const std::vector <DRAWABLE*> & drawlist,
		RENDER_INPUT_SCENE & render_scene,
		RENDER_OUTPUT & render_output,
		std::ostream & error_output);

	void RenderDrawlists(
		const std::vector <DRAWABLE*> & dynamic_drawlist,
		const std::vector <DRAWABLE*> & static_drawlist,
		const std::vector <TEXTURE_INTERFACE*> & extra_textures,
		RENDER_INPUT_SCENE & render_scene,
		RENDER_OUTPUT & render_output,
		std::ostream & error_output);

	void RenderPostProcess(
		const std::string & shadername,
		const std::vector <TEXTURE_INTERFACE*> & textures,
		RENDER_OUTPUT & render_output,
		bool write_color,
		bool write_alpha,
		std::ostream & error_output);

	void Render(
		RENDER_INPUT * input,
		RENDER_OUTPUT & output,
		std::ostream & error_output);

	void DrawBox(
		const MATHVECTOR <float, 3> & corner1,
		const MATHVECTOR <float, 3> & corner2) const;
};

#endif
