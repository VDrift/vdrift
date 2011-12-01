#include "graphics_fallback.h"

#include "opengl_utility.h"
#include "mathvector.h"
#include "texture.h"
#include "vertexarray.h"
#include "reseatable_reference.h"
#include "containeralgorithm.h"
#include "graphics_config.h"

#ifdef __APPLE__
#include <GLEW/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <cassert>

#include <sstream>
using std::stringstream;

#include <string>
using std::string;

#include <iostream>
using std::pair;
using std::endl;

#include <map>
using std::map;

#include <vector>
using std::vector;

#include <algorithm>

//#define _SHADOWMAP_DEBUG_
//#define _DYNAMIC_REFLECT_DEBUG_

///break up the input into a vector of strings using the token characters given
std::vector <std::string> Tokenize(const std::string & input, const std::string & tokens)
{
	std::vector <std::string> out;

	unsigned int pos = 0;
	unsigned int lastpos = 0;

	while (pos != (unsigned int) std::string::npos)
	{
		pos = input.find_first_of(tokens, pos);
		string thisstr = input.substr(lastpos,pos-lastpos);
		if (!thisstr.empty())
			out.push_back(thisstr);
		pos = input.find_first_not_of(tokens, pos);
		lastpos = pos;
	}

	return out;
}

void ReportOnce(const void * id, const std::string & message, std::ostream & output)
{
	static std::map <const void*, std::string> prev_messages;

	std::map <const void*, std::string>::iterator i = prev_messages.find(id);
	if (i == prev_messages.end() || i->second != message)
	{
		prev_messages[id] = message;
		output << message << std::endl;
	}
}

bool GRAPHICS_FALLBACK::Init(const std::string & shaderpath,
	unsigned int resx, unsigned int resy, unsigned int bpp,
	unsigned int depthbpp, bool fullscreen, bool shaders,
	unsigned int antialiasing, bool enableshadows, int new_shadow_distance,
	int new_shadow_quality, int reflection_type,
	const std::string & static_reflectionmap_file,
	const std::string & static_ambientmap_file,
	int anisotropy, int texturesize,
	int lighting_quality, bool newbloom, bool newnormalmaps,
	const std::string & renderconfig,
	std::ostream & info_output, std::ostream & error_output)
{
	shadows = enableshadows;
	shadow_distance = new_shadow_distance;
	shadow_quality = new_shadow_quality;
	lighting = lighting_quality;
	bloom = newbloom;
	normalmaps = newnormalmaps;
	renderconfigfile = renderconfig;

	if (reflection_type == 1)
		reflection_status = REFLECTION_STATIC;
	else if (reflection_type == 2)
		reflection_status = REFLECTION_DYNAMIC;

	ChangeDisplay(resx, resy, bpp, depthbpp, fullscreen, antialiasing, info_output, error_output);

	fsaa = 1;
	if (antialiasing > 1)
		fsaa = antialiasing;

	stringstream cardinfo;
	cardinfo << "Video card information:" << endl;
	cardinfo << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cardinfo << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cardinfo << "Version: " << glGetString(GL_VERSION) << endl;
	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	cardinfo << "Maximum texture size: " << texSize << endl;
	GLint maxfloats(0);
	glGetIntegerv(GL_MAX_VARYING_FLOATS_ARB, &maxfloats);
	cardinfo << "Maximum varying floats: " << maxfloats << endl;
	info_output << cardinfo.str();
	if (cardinfo.str().find("NVIDIA") == string::npos &&
		cardinfo.str().find("ATI") == string::npos &&
		cardinfo.str().find("AMD") == string::npos)
	{
		error_output << "You don't have an NVIDIA or ATI/AMD card.  This game may not run correctly or at all." << endl;
	}

	std::stringstream vendorstr;
	vendorstr << glGetString(GL_VENDOR);
	if (vendorstr.str().find("ATI") != string::npos ||
		vendorstr.str().find("AMD") != string::npos)
	{
		aticard = true;
	}

	//initialize GLEW
	//glewExperimental = GL_TRUE; // expose all avaiable extensions
	GLenum glew_err = glewInit();
	if ( glew_err != GLEW_OK )
	{
		error_output << "GLEW failed to initialize: " << glewGetErrorString ( glew_err ) << endl;
		assert(glew_err == GLEW_OK);
	}
	else
	{
		info_output << "Using GLEW " << glewGetString ( GLEW_VERSION ) << endl;
	}

	if (!GLEW_ARB_multitexture)
	{
		info_output << "Your video card doesn't support multitexturing.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_texture_cube_map)
	{
		info_output << "Your video card doesn't support cube maps.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_framebuffer_object)
	{
		info_output << "Your video card doesn't support framebuffer objects.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_draw_buffers)
	{
		info_output << "Your video card doesn't support multiple draw buffers.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_texture_non_power_of_two)
	{
		info_output << "Your video card doesn't support non-power-of-two textures.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_texture_float)
	{
		info_output << "Your video card doesn't support floating point textures.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_half_float_pixel)
	{
		info_output << "Your video card doesn't support 16-bit floats.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else if (!GLEW_ARB_shader_texture_lod && !GLEW_VERSION_2_1) // texture2DLod in logluminance shader
	{
		info_output << "Your video card doesn't support texture2DLod.  Disabling shaders." << endl;
		DisableShaders(shaderpath, error_output);
	}
	else
	{
		GLint maxattach;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxattach);
		info_output << "Maximum color attachments: " << maxattach << endl;

		const GLint reqmrt = 1;

		GLint mrt;
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &mrt);
		info_output << "Maximum draw buffers (" << reqmrt << " required): " << mrt << endl;

		if (GLEW_ARB_shading_language_100 && GLEW_VERSION_2_0 && shaders && GLEW_ARB_fragment_shader && mrt >= reqmrt && maxattach >= reqmrt)
		{
			EnableShaders(shaderpath, info_output, error_output);
		}
		else
		{
			info_output << "Disabling shaders" << endl;
			DisableShaders(shaderpath, error_output);
		}
	}

	//load static reflection map for dynamic reflections too, since we may need it
	if ((reflection_status == REFLECTION_STATIC || reflection_status == REFLECTION_DYNAMIC) && !static_reflectionmap_file.empty())
	{
		TEXTUREINFO t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = true;
		t.anisotropy = anisotropy;
		t.maxsize = TEXTUREINFO::Size(texturesize);
		static_reflection.Load(static_reflectionmap_file, t, error_output);
	}

	if (!static_ambientmap_file.empty())
	{
		TEXTUREINFO t;
		t.cube = true;
		t.verticalcross = true;
		t.mipmap = false;
		t.anisotropy = anisotropy;
		t.maxsize = TEXTUREINFO::Size(texturesize);
		static_ambient.Load(static_ambientmap_file, t, error_output);
	}

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

	info_output << "Maximum anisotropy: " << max_anisotropy << endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("Shader loading", error_output);

	initialized = true;

	return true;
}

void GRAPHICS_FALLBACK::ChangeDisplay(const int width, const int height, const int bpp, const int dbpp,
				   const bool fullscreen, unsigned int antialiasing,
       				   std::ostream & info_output, std::ostream & error_output)
{
	GLfloat ratio = ( GLfloat )width / ( GLfloat )height;
	glViewport( 0, 0, ( GLint )width, ( GLint )height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 45.0f, ratio, 0.1f, 100.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	OPENGL_UTILITY::CheckForOpenGLErrors("ChangeDisplay", error_output);

	w = width;
	h = height;
}

void GRAPHICS_FALLBACK::Deinit()
{
	if (GLEW_ARB_shading_language_100)
	{
		if (!shadermap.empty())
			glUseProgramObjectARB(0);
		shadermap.clear();
	}
}

void GRAPHICS_FALLBACK::BeginScene(std::ostream & error_output)
{
	glstate.Disable(GL_TEXTURE_2D);
	glShadeModel( GL_SMOOTH );
	glClearColor(0,0,0,0);
	glClearDepth( 1.0f );
	glstate.Enable(GL_DEPTH_TEST);
	glDepthFunc( GL_LEQUAL );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glstate.Disable(GL_LIGHTING);
	glstate.SetColor(0.5,0.5,0.5,1.0);
	glPolygonOffset(-1.0,-1.0);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	OPENGL_UTILITY::CheckForOpenGLErrors("BeginScene", error_output);
}

///note that if variant is passed in, it is used as the shader name and the shader is also loaded with the variant_defines set
///this allows loading multiple shaders from the same shader file, just with different defines set
///variant_defines is a space delimited list of defines
bool GRAPHICS_FALLBACK::LoadShader(const std::string & shaderpath, const std::string & name, std::ostream & info_output, std::ostream & error_output, std::string variant, std::string variant_defines)
{
	//generate preprocessor defines
	std::vector <std::string> defines;

	{
		stringstream s;
		s << "SCREENRESX " << w;
		defines.push_back(s.str());
	}
	{
		stringstream s;
		s << "SCREENRESY " << h;
		defines.push_back(s.str());
	}

	if (reflection_status == REFLECTION_DISABLED)
		defines.push_back("_REFLECTIONDISABLED_");
	else if (reflection_status == REFLECTION_STATIC)
		defines.push_back("_REFLECTIONSTATIC_");
	else if (reflection_status == REFLECTION_DYNAMIC)
		defines.push_back("_REFLECTIONDYNAMIC_");

	if (shadows)
	{
		defines.push_back("_SHADOWS_");
		if (shadow_distance > 0)
			defines.push_back("_CSM2_");
		if (shadow_distance > 1)
			defines.push_back("_CSM3_");
		if (shadow_quality == 0)
			defines.push_back("_SHADOWSLOW_");
		if (shadow_quality == 1)
			defines.push_back("_SHADOWSMEDIUM_");
		if (shadow_quality == 2)
			defines.push_back("_SHADOWSHIGH_");
		if (shadow_quality == 3)
			defines.push_back("_SHADOWSVHIGH_");
		if (shadow_quality == 4)
			defines.push_back("_SHADOWSULTRA_");
	}

	if (normalmaps)
		defines.push_back("_NORMALMAPS_");

	if (lighting == 1)
		defines.push_back("_SSAO_LOW_");

	if (lighting == 2)
		defines.push_back("_SSAO_HIGH_");

	std::string shadername = name;
	if (!variant.empty())
	{
		shadername = variant;
		if (!variant_defines.empty())
		{
			stringstream s(variant_defines);
			while (s)
			{
				std::string newdefine;
				s >> newdefine;
				if (!newdefine.empty())
					defines.push_back(newdefine);
			}
		}
	}
	pair <map < string, SHADER_GLSL >::iterator, bool> result = shadermap.insert(pair < string, SHADER_GLSL > (shadername, SHADER_GLSL()));
	map < string, SHADER_GLSL >::iterator i = result.first;

	bool success = true;
	//if (result.second) //if the insertion resulted in a new object being created, set it up
	{
		success = i->second.Load(shaderpath+"/"+name+"/vertex.glsl", shaderpath+"/"+name+"/fragment.glsl",
					 defines, info_output, error_output);
		if (success)
		{
			info_output << "Loaded shader package "+name;
			if (!variant.empty() && variant != name)
				info_output << ", variant " << variant;
			info_output << endl;
		}
	}

	return success;
}

FBTEXTURE::FORMAT TextureFormatFromString(const std::string & format)
{
	if (format == "depth" || format == "depthshadow")
		return FBTEXTURE::DEPTH24;
	else if (format == "luminance8")
		return FBTEXTURE::LUM8;
	else if (format == "RGBA8")
		return FBTEXTURE::RGBA8;
	else if (format == "RGB8")
		return FBTEXTURE::RGB8;
	else if (format == "RGBA16")
		return FBTEXTURE::RGBA16;
	else if (format == "RGB16")
		return FBTEXTURE::RGB16;
	else
		assert(0);

	return FBTEXTURE::RGB8;
}

void GRAPHICS_FALLBACK::EnableShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output)
{
	bool shader_load_success = true;

	{
		GLint tu;
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS,&tu );
		GLint tufull;
		glGetIntegerv( GL_MAX_TEXTURE_UNITS,&tufull );
		info_output << "Texture units: " << tufull << " full, " << tu << " partial" << std::endl;
	}

	OPENGL_UTILITY::CheckForOpenGLErrors("EnableShaders: start", error_output);

	// unload current shaders
	glUseProgramObjectARB(0);
	for (shader_map_type::iterator i = shadermap.begin(); i != shadermap.end(); i++)
	{
		i->second.Unload();
	}
	shadermap.clear();
	activeshader = shadermap.end();

	OPENGL_UTILITY::CheckForOpenGLErrors("EnableShaders: shader unload", error_output);

	// reload configuration
	config = GRAPHICS_CONFIG();
	std::string rcpath = shaderpath+"/" + renderconfigfile;
	if (!config.Load(rcpath, error_output))
	{
		error_output << "Error loading render configuration file: " << rcpath << std::endl;
		shader_load_success = false;
	}

	// reload shaders
	std::set <std::string> shadernames;
	for (std::vector <GRAPHICS_CONFIG_SHADER>::const_iterator s = config.shaders.begin(); s != config.shaders.end(); s++)
	{
		assert(shadernames.find(s->name) == shadernames.end());
		shadernames.insert(s->name);
		shader_load_success = shader_load_success && LoadShader(shaderpath, s->folder, info_output, error_output, s->name, s->defines);
	}

	OPENGL_UTILITY::CheckForOpenGLErrors("EnableShaders: shader loading", error_output);

	if (shader_load_success)
	{
		using_shaders = true;
		info_output << "Successfully enabled shaders" << endl;

		// unload current outputs
		render_outputs.clear();
		texture_outputs.clear();
		texture_inputs.clear();

		OPENGL_UTILITY::CheckForOpenGLErrors("EnableShaders: FBO deinit", error_output);

		bool ssao = (lighting > 0);
		bool ssao_low = (lighting == 1);
		bool ssao_high = (lighting == 2);
		bool reflection_disabled = (reflection_status == REFLECTION_DISABLED);
		bool reflection_dynamic = (reflection_status == REFLECTION_DYNAMIC);
		bool shadows_near = shadows;
		bool shadows_medium = shadows && shadow_distance > 0;
		bool shadows_far = shadows && shadow_distance > 1;
		bool shadow_quality_low = shadows && (shadow_quality == 0);
		bool shadow_quality_medium = shadows && (shadow_quality == 1);
		bool shadow_quality_high = shadows && (shadow_quality == 2);
		bool shadow_quality_vhigh = shadows && (shadow_quality == 3);
		bool shadow_quality_ultra = shadows && (shadow_quality == 4);

		// for now, map vhigh and ultra to high
		shadow_quality_high = shadow_quality_high || shadow_quality_vhigh || shadow_quality_ultra;
		shadow_quality_vhigh = false;
		shadow_quality_ultra = true;

		conditions.clear();
		if (fsaa > 1) conditions.insert("fsaa");
		#define ADDCONDITION(x) if (x) conditions.insert(#x)
		ADDCONDITION(bloom);
		ADDCONDITION(normalmaps);
		ADDCONDITION(ssao);
		ADDCONDITION(ssao_low);
		ADDCONDITION(ssao_high);
		ADDCONDITION(reflection_disabled);
		ADDCONDITION(reflection_dynamic);
		ADDCONDITION(shadows_near);
		ADDCONDITION(shadows_medium);
		ADDCONDITION(shadows_far);
		ADDCONDITION(shadow_quality_low);
		ADDCONDITION(shadow_quality_medium);
		ADDCONDITION(shadow_quality_high);
		ADDCONDITION(shadow_quality_vhigh);
// 		ADDCONDITION(shadow_quality_ultra);
		#undef ADDCONDITION

		// add some common textures
		if (reflection_status == REFLECTION_STATIC)
			texture_inputs["reflection_cube"] = static_reflection;
		texture_inputs["ambient_cube"] = static_ambient;

		for (std::vector <GRAPHICS_CONFIG_OUTPUT>::const_iterator i = config.outputs.begin(); i != config.outputs.end(); i++)
		{
			if (i->conditions.Satisfied(conditions))
			{
				if (texture_outputs.find(i->name) != texture_outputs.end())
				{
					error_output << "Detected duplicate output name in render config: " << i->name << ", only the first output will be constructed." << std::endl;
					continue;
				}

				if (i->type == "framebuffer")
				{
					render_outputs[i->name].RenderToFramebuffer();
				}
				else
				{
					FBTEXTURE & fbtex = texture_outputs[i->name];
					FBTEXTURE::TARGET type = FBTEXTURE::NORMAL;
					if (i->type == "rectangle")
						type = FBTEXTURE::RECTANGLE;
					else if (i->type == "cube")
						type = FBTEXTURE::CUBEMAP;
					int fbms = 0;
					if (i->multisample < 0)
						fbms = fsaa;

					// initialize fbtexture
					fbtex.Init(glstate,
							   i->width.GetSize(w),
							   i->height.GetSize(h),
							   type,
							   TextureFormatFromString(i->format),
							   (i->filter == "nearest"),
							   i->mipmap,
							   error_output,
							   fbms,
							   (i->format == "depthshadow"));

					// map to input texture
					texture_inputs[i->name] = fbtex;
				}

				info_output << "Initialized render output: " << i->name << (i->type != "framebuffer" ? " (FBO)" : " (framebuffer alias)") << std::endl;
			}
		}

		render_outputs["framebuffer"].RenderToFramebuffer();

		// go through all pass outputs and construct the actual FBOs, which can consist of one or more fbtextures
		for (std::vector <GRAPHICS_CONFIG_PASS>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
		{
			if (i->conditions.Satisfied(conditions))
			{
				// see if it already exists
				std::string outname = i->output;
				render_output_map_type::iterator curout = render_outputs.find(outname);
				if (curout == render_outputs.end())
				{
					// tokenize the output list
					std::vector <std::string> outputs = Tokenize(outname, " ");

					// collect a list of textures for the outputs
					std::vector <FBTEXTURE*> fbotex;
					for (std::vector <std::string>::const_iterator o = outputs.begin(); o != outputs.end(); o++)
					{
						texture_output_map_type::iterator to = texture_outputs.find(*o);
						if (to != texture_outputs.end())
						{
							fbotex.push_back(&to->second);
						}
					}

					if (fbotex.empty())
					{
						error_output << "None of these outputs are active: " << error_output << ", this pass will not have an output." << std::endl;
						continue;
					}

					// initialize fbo
					FBOBJECT & fbo = render_outputs[outname].RenderToFBO();
					fbo.Init(glstate, fbotex, error_output);
				}
			}
		}
	}
	else
	{
		error_output << "Disabling shaders due to shader loading error" << endl;
		DisableShaders(shaderpath, error_output);
	}
}

bool GRAPHICS_FALLBACK::ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output)
{
	EnableShaders(shaderpath, info_output, error_output);

	return GetUsingShaders();
}

void GRAPHICS_FALLBACK::DisableShaders(const std::string & shaderpath, std::ostream & error_output)
{
	shadermap.clear();
	using_shaders = false;
	shadows = false;

	if (GLEW_ARB_shading_language_100)
	{
		glUseProgramObjectARB(0);
	}

	// load non-shader configuration
	config = GRAPHICS_CONFIG();
	std::string rcpath = shaderpath+"/render.conf.noshaders";
	if (!config.Load(rcpath, error_output))
	{
		error_output << "Error loading non-shader render configuration file: " << rcpath << std::endl;

		// uh oh, now we're really boned
		assert(0);
	}

	render_outputs["framebuffer"].RenderToFramebuffer();
}

void GRAPHICS_FALLBACK::EndScene(std::ostream & error_output)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("EndScene", error_output);
}

void GRAPHICS_FALLBACK::SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
					const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos)
{
	// setup the default camera from the passed-in parameters
	{
		GRAPHICS_CAMERA & cam = cameras["default"];
		cam.fov = fov;
		cam.pos = cam_position;
		cam.orient = cam_rotation;
		cam.view_distance = new_view_distance;
		cam.w = w;
		cam.h = h;
	}

	// create a camera for the skybox with a long view distance
	{
		GRAPHICS_CAMERA & cam = cameras["skybox"];
		cam = cameras["default"];
		cam.view_distance = 10000.0;
	}

	// create a camera for 3d ui elements that has a fixed FOV
	{
		GRAPHICS_CAMERA & cam = cameras["ui3d"];
		cam.fov = 45;
		cam.pos = cam_position;
		cam.orient = cam_rotation;
		cam.view_distance = new_view_distance;
		cam.w = w;
		cam.h = h;
	}

	// create a camera for the dynamic reflections
	{
		GRAPHICS_CAMERA & cam = cameras["dynamic_reflection"];
		cam.pos = dynamic_reflection_sample_pos;
		cam.fov = 90; // this gets automatically overridden with the correct fov (which is 90 anyway)
		cam.orient.LoadIdentity(); // this gets automatically rotated for each cube side
		cam.view_distance = 100.f;
		cam.w = 1.f; // this gets automatically overridden with the cubemap dimensions
		cam.h = 1.f; // this gets automatically overridden with the cubemap dimensions
	}

	// create a camera for the dynamic reflection skybox
	{
		GRAPHICS_CAMERA & cam = cameras["dynamic_reflection_skybox"];
		cam = cameras["dynamic_reflection"];
		cam.view_distance = 10000.f;
	}

	// create an ortho camera for 2d drawing
	{
		GRAPHICS_CAMERA & cam = cameras["2d"];

		// this is the glOrtho call we want:
		//glOrtho( 0, 1, 1, 0, -1, 1 );

		cam.orthomode = true;
		cam.orthomin = MATHVECTOR <float, 3> (0,1,-1);
		cam.orthomax = MATHVECTOR <float, 3> (1,0,1);
	}

	// create cameras for shadow passes
	if (shadows)
	{
		std::vector <std::string> shadow_names;
		shadow_names.push_back("near");
		shadow_names.push_back("medium");
		shadow_names.push_back("far");
		for (int i = 0; i < 3; i++)
		{
			float shadow_radius = (1<<i)*closeshadow+(i)*20.0; //5,30,60

			MATHVECTOR <float, 3> shadowbox(1,1,1);
			shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
			MATHVECTOR <float, 3> shadowoffset(0,0,-1);
			shadowoffset = shadowoffset * shadow_radius;
			(-cam_rotation).RotateVector(shadowoffset);
			shadowbox[2] += 60.0;

			GRAPHICS_CAMERA & cam = cameras["shadows_"+shadow_names[i]];
			cam = cameras["default"];
			cam.orthomode = true;
			cam.orthomin = -shadowbox;
			cam.orthomax = shadowbox;
			cam.pos = cam.pos + shadowoffset;
			cam.orient = lightdirection;

			// go through and extract the clip matrix, storing it in a texture matrix
			renderscene.SetOrtho(cam.orthomin, cam.orthomax);
			renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h, false);
			float mv[16], mp[16], clipmat[16];
			glGetFloatv( GL_PROJECTION_MATRIX, mp );
			glGetFloatv( GL_MODELVIEW_MATRIX, mv );
			glMatrixMode( GL_TEXTURE );
			glPushMatrix();
			glLoadIdentity();
			glTranslatef (0.5, 0.5, 0.5);
			glScalef (0.5, 0.5, 0.5);
			glMultMatrixf(mp);
			glMultMatrixf(mv);
			glGetFloatv(GL_TEXTURE_MATRIX, clipmat);
			glPopMatrix();
			glMatrixMode( GL_MODELVIEW );
			glActiveTexture(GL_TEXTURE4+i);
			glMatrixMode( GL_TEXTURE );
			glLoadMatrixf(clipmat);
			glMatrixMode( GL_MODELVIEW );
			glActiveTexture(GL_TEXTURE0);
		}
	}
}

static bool SortDraworder(DRAWABLE * d1, DRAWABLE * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

std::string BuildKey(const std::string & camera, const std::string & draw)
{
	return camera + ";" + draw;
}

QUATERNION <float> GetCubeSideOrientation(int i, const QUATERNION <float> & origorient, std::ostream & error_output)
{
	QUATERNION <float> orient = origorient;

	switch (i)
	{
		case 0:
		orient.Rotate(M_PI*0.5, 0,1,0);
		break;

		case 1:
		orient.Rotate(-M_PI*0.5, 0,1,0);
		break;

		case 2:
		orient.Rotate(M_PI*0.5, 1,0,0);
		break;

		case 3:
		orient.Rotate(-M_PI*0.5, 1,0,0);
		break;

		case 4:
		// orient is already set up for us!
		break;

		case 5:
		orient.Rotate(M_PI, 0,1,0);
		break;

		default:
		error_output << "Reached odd spot while building cubemap orientation. How many sides are in a cube, anyway? " << i << "?" << std::endl;
		assert(0);
		break;
	};

	return orient;
}

void AttachCubeSide(int i, FBOBJECT & reflection_fbo, std::ostream & error_output)
{
	switch (i)
	{
		case 0:
		reflection_fbo.SetCubeSide(FBTEXTURE::POSX);
		break;

		case 1:
		reflection_fbo.SetCubeSide(FBTEXTURE::NEGX);
		break;

		case 2:
		reflection_fbo.SetCubeSide(FBTEXTURE::POSY);
		break;

		case 3:
		reflection_fbo.SetCubeSide(FBTEXTURE::NEGY);
		break;

		case 4:
		reflection_fbo.SetCubeSide(FBTEXTURE::POSZ);
		break;

		case 5:
		reflection_fbo.SetCubeSide(FBTEXTURE::NEGZ);
		break;

		default:
		error_output << "Reached odd spot while attaching cubemap side. How many sides are in a cube, anyway? " << i << "?" << std::endl;
		assert(0);
		break;
	};

	OPENGL_UTILITY::CheckForOpenGLErrors("cubemap generation: FBO cube side attachment", error_output);
}

GLint DepthModeFromString(const std::string & mode)
{
	if (mode == "lequal")
		return GL_LEQUAL;
	else if (mode == "equal")
		return GL_EQUAL;
	else if (mode == "gequal")
		return GL_GEQUAL;
	else if (mode == "disabled")
		return GL_ALWAYS;
	else
		assert(0);

	return GL_LEQUAL;
}

BLENDMODE::BLENDMODE BlendModeFromString(const std::string & mode)
{
	if (mode == "disabled")
		return BLENDMODE::DISABLED;
	else if (mode == "add")
		return BLENDMODE::ADD;
	else if (mode == "alphablend")
		return BLENDMODE::ALPHABLEND;
	else if (mode == "alphablend_premultiplied")
		return BLENDMODE::PREMULTIPLIED_ALPHA;
	else if (mode == "alphatest")
		return BLENDMODE::ALPHATEST;
	else
		assert(0);

	return BLENDMODE::DISABLED;
}

void GRAPHICS_FALLBACK::DrawScene(std::ostream & error_output)
{
	renderscene.SetFlags(using_shaders);
	renderscene.SetFSAA(fsaa);

	if (reflection_status == REFLECTION_STATIC)
		renderscene.SetReflection(&static_reflection);
	renderscene.SetAmbient(static_ambient);
	renderscene.SetContrast(contrast);
	postprocess.SetContrast(contrast);

	//sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);

	//do fast culling queries for static geometry, but only where necessary
	//for each pass, we have which camera and which draw layer to use
	//we want to do culling for each unique camera and draw layer combination
	//use camera/layer as the unique key
	std::map <std::string, PTRVECTOR <DRAWABLE> > culled_static_drawlist;
	for (std::vector <GRAPHICS_CONFIG_PASS>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
	{
		assert(!i->draw.empty());
		if (i->draw.back() != "postprocess" && i->conditions.Satisfied(conditions))
		{
			for (std::vector <std::string>::const_iterator d = i->draw.begin(); d != i->draw.end(); d++)
			{
				// determine if we're dealing with a cubemap
				render_output_map_type::iterator oi = render_outputs.find(i->output);

				if (oi == render_outputs.end())
				{
					ReportOnce(&*i, "Render output "+i->output+" couldn't be found", error_output);
					continue;
				}

				bool cubemap = (oi->second.IsFBO() && oi->second.RenderToFBO().IsCubemap());

				std::string cameraname = i->camera;
				const int cubesides = cubemap ? 6 : 1;

				for (int cubeside = 0; cubeside < cubesides; cubeside++)
				{
					if (cubemap)
					{
						// build sub-camera

						// build a name for the sub camera
						{
							std::stringstream converter;
							converter << i->camera << "_cubeside" << cubeside;
							cameraname = converter.str();
						}

						// get the base camera
						camera_map_type::iterator bci = cameras.find(i->camera);

						if (bci == cameras.end())
						{
							ReportOnce(&*i, "Camera "+i->camera+" couldn't be found", error_output);
							continue;
						}

						// create our sub-camera
						GRAPHICS_CAMERA & cam = cameras[cameraname];
						cam = bci->second;

						// set the sub-camera's properties
						cam.orient = GetCubeSideOrientation(cubeside, cam.orient, error_output);
						cam.fov = 90;
						assert(oi->second.IsFBO());
						const FBOBJECT & fbo = oi->second.RenderToFBO();
						cam.w = fbo.GetWidth();
						cam.h = fbo.GetHeight();
					}

					std::string key = BuildKey(cameraname, *d);
					if (i->cull)
					{
						camera_map_type::iterator ci = cameras.find(cameraname);

						if (ci == cameras.end())
						{
							ReportOnce(&*i, "Camera "+cameraname+" couldn't be found", error_output);
							continue;
						}

						GRAPHICS_CAMERA & cam = ci->second;
						if (culled_static_drawlist.find(key) == culled_static_drawlist.end())
						{
							if (cam.orthomode)
								renderscene.SetOrtho(cam.orthomin, cam.orthomax);
							else
								renderscene.DisableOrtho();
							FRUSTUM frustum = renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
							reseatable_reference <AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> > container =
								static_drawlist.GetDrawlist().GetByName(*d);

							if (!container)
							{
								ReportOnce(&*i, "Drawable container "+*d+" couldn't be found", error_output);
								continue;
							}

							container->Query(frustum, culled_static_drawlist[key]);
							renderscene.DisableOrtho();
						}
					}
					else
					{
						reseatable_reference <AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> > container =
							static_drawlist.GetDrawlist().GetByName(*d);

						if (!container)
						{
							ReportOnce(&*i, "Drawable container "+*d+" couldn't be found", error_output);
							continue;
						}

						container->Query(AABB<float>::INTERSECT_ALWAYS(), culled_static_drawlist[key]);
					}
				}
			}
		}
	}

	//construct light position
	MATHVECTOR <float, 3> lightposition(0,0,1);
	(-lightdirection).RotateVector(lightposition);
	renderscene.SetSunDirection(lightposition);
	postprocess.SetSunDirection(lightposition);

	// draw the passes
	for (std::vector <GRAPHICS_CONFIG_PASS>::const_iterator i = config.passes.begin(); i != config.passes.end(); i++)
	{
		if (i->conditions.Satisfied(conditions))
		{
			// convert "inputs" into a vector form
			std::vector <TEXTURE_INTERFACE*> input_textures;
			for (std::map <unsigned int, std::string>::const_iterator t = i->inputs.tu.begin(); t != i->inputs.tu.end(); t++)
			{
				unsigned int tuid = t->first;

				unsigned int cursize = input_textures.size();
				for (unsigned int extra = cursize; extra < tuid; extra++)
					input_textures.push_back(NULL);

				std::string texname = t->second;

				// quietly ignore invalid names
				// this allows us to specify outputs that are only present for certain conditions
				// and then always specify those outputs as inputs to later stages, and have
				// them be ignored if the conditions aren't met
				if (texture_inputs.find(texname) != texture_inputs.end())
				{
					input_textures.push_back(&*texture_inputs[texname]);
				}
				else
				{
					//TODO: decide if i want to do fancier error detection here to catch typos in render.conf
					//std::cout << "warning: " << texname << " not found" << std::endl;
					input_textures.push_back(NULL);
				}
			}

			assert(!i->draw.empty());
			if (i->draw.back() == "postprocess")
			{
				// setup camera, even though we don't use it directly for the post process we want to have some info available
				std::string cameraname = i->camera;
				camera_map_type::iterator ci = cameras.find(cameraname);
				if (ci == cameras.end())
				{
					ReportOnce(&*i, "Camera "+cameraname+" couldn't be found", error_output);
					continue;
				}
				GRAPHICS_CAMERA & cam = ci->second;
				if (cam.orthomode)
					renderscene.SetOrtho(cam.orthomin, cam.orthomax);
				else
					renderscene.DisableOrtho();
				renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
				postprocess.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h); //so we have this later if we want

				postprocess.SetDepthMode(DepthModeFromString(i->depthtest));
				postprocess.SetWriteDepth(i->write_depth);
				postprocess.SetClear(i->clear_color, i->clear_depth);
				postprocess.SetBlendMode(BlendModeFromString(i->blendmode));

				shader_map_type::iterator si = shadermap.find(i->shader);
				if (si == shadermap.end())
				{
					ReportOnce(&*i, "Shader "+i->shader+" couldn't be found", error_output);
					continue;
				}

				RenderPostProcess(i->shader, input_textures, render_outputs[i->output], i->write_color, i->write_alpha, error_output);
			}
			else
			{
				renderscene.SetBlendMode(BlendModeFromString(i->blendmode));
				renderscene.SetDepthMode(DepthModeFromString(i->depthtest));

				for (std::vector <std::string>::const_iterator d = i->draw.begin(); d != i->draw.end(); d++)
				{
					// setup render output
					render_output_map_type::iterator oi = render_outputs.find(i->output);

					if (oi == render_outputs.end())
					{
						ReportOnce(&*i, "Render output "+i->output+" couldn't be found", error_output);
						continue;
					}

					// handle the cubemap case
					bool cubemap = (oi->second.IsFBO() && oi->second.RenderToFBO().IsCubemap());
					std::string cameraname = i->camera;
					const int cubesides = cubemap ? 6 : 1;

					for (int cubeside = 0; cubeside < cubesides; cubeside++)
					{
						if (cubemap)
						{
							// build a name for the sub camera
							std::stringstream converter;
							converter << i->camera << "_cubeside" << cubeside;
							cameraname = converter.str();

							// attach the correct cube side on the render output
							AttachCubeSide(cubeside, oi->second.RenderToFBO(), error_output);
						}

						// setup camera
						camera_map_type::iterator ci = cameras.find(cameraname);

						if (ci == cameras.end())
						{
							ReportOnce(&*i, "Camera "+i->camera+" couldn't be found", error_output);
							continue;
						}

						GRAPHICS_CAMERA & cam = ci->second;
						if (cam.orthomode)
							renderscene.SetOrtho(cam.orthomin, cam.orthomax);
						else
							renderscene.DisableOrtho();
						renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);

						// setup shader
						if (using_shaders)
						{
							shader_map_type::iterator si = shadermap.find(i->shader);
							if (si == shadermap.end())
							{
								ReportOnce(&*i, "Shader "+i->shader+" couldn't be found", error_output);
								continue;
							}
							renderscene.SetDefaultShader(si->second);
						}

						// setup other flags
						if (d == i->draw.begin())
							renderscene.SetClear(i->clear_color, i->clear_depth);
						else
							renderscene.SetClear(false, false);
						renderscene.SetWriteColor(i->write_color);
						renderscene.SetWriteAlpha(i->write_alpha);
						renderscene.SetWriteDepth(i->write_depth);

						// setup dynamic drawlist
						reseatable_reference <PTRVECTOR <DRAWABLE> > container = dynamic_drawlist.GetByName(*d);

						if (!container)
						{
							ReportOnce(&*i, "Drawable container "+*d+" couldn't be found", error_output);
							continue;
						}

						// setup static drawlist
						std::map <std::string, PTRVECTOR <DRAWABLE> >::iterator container_static =
									culled_static_drawlist.find(BuildKey(cameraname,*d));

						if (container_static == culled_static_drawlist.end())
						{
							ReportOnce(&*i, "Couldn't find culled static drawlist for camera/draw combination: "+BuildKey(cameraname,*d), error_output);
							continue;
						}

						OPENGL_UTILITY::CheckForOpenGLErrors("render setup", error_output);

						// car paint hack for non-shader path
						if (!using_shaders && (*d == "nocamtrans_noblend" || *d == "car_noblend"))
						{
							renderscene.SetCarPaintHack(true);
						}
						else
							renderscene.SetCarPaintHack(false);

						// render
						RenderDrawlists(*container,
											container_static->second,
											input_textures,
											renderscene,
											oi->second,
											error_output);

						// cleanup
						renderscene.DisableOrtho();
					}
				}
			}
		}
	}
}

void GRAPHICS_FALLBACK::RenderDrawlist(std::vector <DRAWABLE*> & drawlist,
						RENDER_INPUT_SCENE & render_scene,
						RENDER_OUTPUT & render_output,
						std::ostream & error_output)
{
	if (drawlist.empty() && !render_scene.GetClear().first && !render_scene.GetClear().second)
		return;
	std::vector <DRAWABLE*> empty;
	render_scene.SetDrawLists(drawlist, empty);
	Render(&render_scene, render_output, error_output);
}

void GRAPHICS_FALLBACK::RenderDrawlists(std::vector <DRAWABLE*> & dynamic_drawlist,
						std::vector <DRAWABLE*> & static_drawlist,
						const std::vector <TEXTURE_INTERFACE*> & extra_textures,
						RENDER_INPUT_SCENE & render_scene,
						RENDER_OUTPUT & render_output,
						std::ostream & error_output)
{
	if (dynamic_drawlist.empty() && static_drawlist.empty() && !render_scene.GetClear().first && !render_scene.GetClear().second)
		return;

	OPENGL_UTILITY::CheckForOpenGLErrors("RenderDrawlists start", error_output);

	for (unsigned int i = 0; i < extra_textures.size(); i++)
	{
		if (extra_textures[i])
		{
			glActiveTexture(GL_TEXTURE0+i);
			extra_textures[i]->Activate();

			if (OPENGL_UTILITY::CheckForOpenGLErrors("RenderDrawlists extra texture bind", error_output))
			{
				error_output << "this error occurred while binding texture " << i << ": id=" << extra_textures[i]->GetID() << " loaded=" << extra_textures[i]->Loaded() << std::endl;
			}
		}
	}

	glActiveTexture(GL_TEXTURE0);

	render_scene.SetDrawLists(dynamic_drawlist, static_drawlist);

	OPENGL_UTILITY::CheckForOpenGLErrors("RenderDrawlists SetDrawLists", error_output);

	Render(&render_scene, render_output, error_output);

	for (unsigned int i = 0; i < extra_textures.size(); i++)
	{
		if (extra_textures[i])
		{
			glActiveTexture(GL_TEXTURE0+i);
			extra_textures[i]->Deactivate();

			if (OPENGL_UTILITY::CheckForOpenGLErrors("RenderDrawlists extra texture unbind", error_output))
			{
				error_output << "this error occurred while binding texture " << i << ": id=" << extra_textures[i]->GetID() << " loaded=" << extra_textures[i]->Loaded() << std::endl;
			}
		}
	}

	glActiveTexture(GL_TEXTURE0);
}

void GRAPHICS_FALLBACK::RenderPostProcess(const std::string & shadername,
						const std::vector <TEXTURE_INTERFACE*> & textures,
						RENDER_OUTPUT & render_output,
						bool write_color,
						bool write_alpha,
						std::ostream & error_output)
{
	postprocess.SetWriteColor(write_color);
	postprocess.SetWriteAlpha(write_alpha);
	std::map <std::string, SHADER_GLSL>::iterator s = shadermap.find(shadername);
	assert(s != shadermap.end());
	postprocess.SetShader(&s->second);
	postprocess.SetSourceTextures(textures);
	Render(&postprocess, render_output, error_output);
}

void GRAPHICS_FALLBACK::DrawBox(const MATHVECTOR <float, 3> & corner1, const MATHVECTOR <float, 3> & corner2) const
{
	//enforce corner order
	MATHVECTOR <float, 3> corner_max;
	MATHVECTOR <float, 3> corner_min;
	for (int i = 0; i < 3; i++)
	{
		if (corner1[i] >= corner2[i])
		{
			corner_max[i] = corner1[i];
			corner_min[i] = corner2[i];
		}
		else
		{
			corner_max[i] = corner2[i];
			corner_min[i] = corner1[i];
		}
	}

	glBegin(GL_QUADS);
	// Front Face
	glNormal3f( 0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_min[0], corner_min[1], corner_max[2]);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_max[0], corner_min[1], corner_max[2]);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_max[0], corner_max[1], corner_max[2]);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_min[0], corner_max[1], corner_max[2]);
	// Back Face
	glNormal3f( 0.0f, 0.0f,-1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_min[0], corner_min[1], corner_min[2]);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_min[0], corner_max[1], corner_min[2]);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_max[0], corner_max[1], corner_min[2]);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_max[0], corner_min[1], corner_min[2]);
	// Top Face
	glNormal3f( 0.0f, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_min[0], corner_max[1], corner_min[2]);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_min[0], corner_max[1], corner_max[2]);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_max[0], corner_max[1], corner_max[2]);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_max[0], corner_max[1], corner_min[2]);
	// Bottom Face
	glNormal3f( 0.0f,-1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_min[0], corner_min[1], corner_min[2]);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_max[0], corner_min[1], corner_min[2]);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_max[0], corner_min[1], corner_max[2]);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_min[0], corner_min[1], corner_max[2]);
	// Right face
	glNormal3f( 1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_max[0], corner_min[1], corner_min[2]);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_max[0], corner_max[1], corner_min[2]);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_max[0], corner_max[1], corner_max[2]);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_max[0], corner_min[1], corner_max[2]);
	// Left Face
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(corner_min[0], corner_min[1], corner_min[2]);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(corner_min[0], corner_min[1], corner_max[2]);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(corner_min[0], corner_max[1], corner_max[2]);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(corner_min[0], corner_max[1], corner_min[2]);
	glEnd();
}

void GRAPHICS_FALLBACK::Render(RENDER_INPUT * input, RENDER_OUTPUT & output, std::ostream & error_output)
{
	output.Begin(glstate, error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output begin", error_output);

	input->Render(glstate, error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render finish", error_output);

	output.End(glstate, error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output end", error_output);
}

void GRAPHICS_FALLBACK::AddStaticNode(SCENENODE & node, bool clearcurrent)
{
	static_drawlist.Generate(node, clearcurrent);
}

bool TextureSort(const DRAWABLE & draw1, const DRAWABLE & draw2)
{
	return (draw1.GetDiffuseMap()->GetID() < draw2.GetDiffuseMap()->GetID());
}
