#include "graphics.h"

#ifdef __APPLE__
#include <GLExtensionWrangler/glew.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "opengl_utility.h"
#include "matrix4.h"
#include "mathvector.h"
#include "model.h"
#include "texture.h"
#include "vertexarray.h"
#include "reseatable_reference.h"
#include "definitions.h"
#include "containeralgorithm.h"
#include "graphics_config.h"

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

void GRAPHICS_SDLGL::Init(const std::string shaderpath, const std::string & windowcaption,
	unsigned int resx, unsigned int resy, unsigned int bpp,
	unsigned int depthbpp, bool fullscreen, bool shaders,
 	unsigned int antialiasing, bool enableshadows, int new_shadow_distance,
  	int new_shadow_quality, int reflection_type,
 	const std::string & static_reflectionmap_file,
  	const std::string & static_ambientmap_file,
 	int anisotropy, const std::string & texturesize,
  	int lighting_quality, bool newbloom,
	std::ostream & info_output, std::ostream & error_output)
{
	shadows = enableshadows;
	shadow_distance = new_shadow_distance;
	shadow_quality = new_shadow_quality;
	lighting = lighting_quality;
	bloom = newbloom;

	if (reflection_type == 1)
		reflection_status = REFLECTION_STATIC;
	else if (reflection_type == 2)
		reflection_status = REFLECTION_DYNAMIC;

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK
/*#ifdef ENABLE_FORCE_FEEDBACK
			| SDL_INIT_HAPTIC
#endif*/
				 ) < 0 )
	{
		string err = SDL_GetError();
		error_output << "SDL initialization failed: " << err << endl;
		assert(0); //die
	}
	else
		info_output << "SDL initialization successful" << endl;

	//OPENGL_UTILITY::CheckForOpenGLErrors("SDL initialization", error_output);

	//override depth bpp if shadows are enabled
	if (shadows && depthbpp != 24)
	{
		info_output << "Automatictally setting depth buffer to 24-bit because shadows are enabled" << endl;
		depthbpp = 24;
	}

	ChangeDisplay(resx, resy, bpp, depthbpp, fullscreen, antialiasing, info_output, error_output);

	SDL_WM_SetCaption(windowcaption.c_str(), NULL);

	stringstream cardinfo;
	cardinfo << "Video card information:" << endl;
	cardinfo << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cardinfo << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cardinfo << "Version: " << glGetString(GL_VERSION) << endl;
	GLint texSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
	cardinfo << "Maximum texture size: " << texSize << endl;
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
		DisableShaders();
	}
	else if (!GLEW_ARB_texture_cube_map)
	{
		info_output << "Your video card doesn't support cube maps.  Disabling shaders." << endl;
		DisableShaders();
	}
	else if (!GLEW_EXT_framebuffer_object)
	{
		info_output << "Your video card doesn't support framebuffer objects.  Disabling shaders." << endl;
		DisableShaders();
	}
	else if (!GLEW_ARB_draw_buffers)
	{
		info_output << "Your video card doesn't support multiple draw buffers.  Disabling shaders." << endl;
		DisableShaders();
	}
	else
	{
		GLint mrt;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &mrt);
		info_output << "Maximum color attachments: " << mrt << endl;
		
		if (GLEW_ARB_shading_language_100 && GLEW_VERSION_2_0 && shaders && GLEW_ARB_fragment_shader)
		{
			EnableShaders(shaderpath, info_output, error_output);
		}
		else
		{
			info_output << "Disabling shaders" << endl;
			DisableShaders();
		}
	}

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

	info_output << "Maximum anisotropy: " << max_anisotropy << endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("Shader loading", error_output);

	//load static reflection map for dynamic reflections too, since we may need it
	if ((reflection_status == REFLECTION_STATIC || reflection_status == REFLECTION_DYNAMIC) && !static_reflectionmap_file.empty())
	{
		TEXTUREINFO t;
		t.SetName(static_reflectionmap_file);
		t.SetCube(true, true);
		t.SetMipMap(false);
		t.SetAnisotropy(anisotropy);
		t.SetSize(texturesize);
		static_reflection.Load(t, error_output);
	}
	
	if (!static_ambientmap_file.empty())
	{
		TEXTUREINFO t;
		t.SetName(static_ambientmap_file);
		t.SetCube(true, true);
		t.SetMipMap(false);
		t.SetAnisotropy(anisotropy);
		t.SetSize(texturesize);
		static_ambient.Load(t, error_output);
	}

	initialized = true;
}

void GRAPHICS_SDLGL::ChangeDisplay(const int width, const int height, const int bpp, const int dbpp,
				   const bool fullscreen, unsigned int antialiasing,
       				   std::ostream & info_output, std::ostream & error_output)
{
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();

	if ( !videoInfo )
	{
		string err = SDL_GetError();
		error_output << "SDL video query failed: " << err << endl;
		assert (0);
	}
	else
		info_output << "SDL video query was successful" << endl;

	int videoFlags;
	videoFlags  = SDL_OPENGL;
	videoFlags |= SDL_GL_DOUBLEBUFFER;
	videoFlags |= SDL_HWPALETTE;
	//videoFlags |= SDL_RESIZABLE;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, dbpp );

	fsaa = 1;
	//if (antialiasing > 1 && GLEW_ARB_multisample) //can't check this because OpenGL and GLEW aren't initialized
	if (antialiasing > 1)
	{
		fsaa = antialiasing;
		info_output << "Enabling antialiasing: " << fsaa << "X" << endl;
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, fsaa );
	}
	else
		info_output << "Disabling antialiasing" << endl;

	if (fullscreen)
	{
		videoFlags |= SDL_HWSURFACE|SDL_ANYFORMAT|SDL_FULLSCREEN;
	}
	else
	{
		videoFlags |= SDL_SWSURFACE|SDL_ANYFORMAT;
	}

	//if (SDL_VideoModeOK(game.config.res_x.data, game.config.res_y.data, game.config.bpp.data, videoFlags) != 0)

	if (surface != NULL)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}

	surface = SDL_SetVideoMode(width, height, bpp, videoFlags);

	GLfloat ratio = ( GLfloat )width / ( GLfloat )height;
	glViewport( 0, 0, ( GLint )width, ( GLint )height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( 45.0f, ratio, 0.1f, 100.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity( );

	if (!surface)
	{
		string err = SDL_GetError();
		error_output << "Display change failed: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << endl;
		error_output << "Error: " << err << endl;
		assert (0);
	}
	else
		info_output << "Display change was successful: " << width << "x" << height << "x" << bpp << " " << dbpp << "z fullscreen=" << fullscreen << endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("ChangeDisplay", error_output);

	w = width;
	h = height;
}

void GRAPHICS_SDLGL::Deinit()
{
	if (GLEW_ARB_shading_language_100)
	{
		if (!shadermap.empty())
			glUseProgramObjectARB(0);
		shadermap.clear();
	}
	
	SDL_Quit();
}

void GRAPHICS_SDLGL::BeginScene(std::ostream & error_output)
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
bool GRAPHICS_SDLGL::LoadShader(const std::string & shaderpath, const std::string & name, std::ostream & info_output, std::ostream & error_output, std::string variant, std::string variant_defines)
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

	if (lighting == 1)
		defines.push_back("_EDGECONTRASTENHANCEMENT_");

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

void GRAPHICS_SDLGL::EnableShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output)
{
	bool shader_load_success = true;
	
	{
		GLint tu;
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS_ARB,&tu );
		GLint tufull;
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB,&tufull );
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
	std::string renderconfigfile = shaderpath+"/render.conf";
	if (!config.Load(renderconfigfile, error_output))
	{
		error_output << "Error loading render configuration file: " << renderconfigfile << std::endl;
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
		for (std::map <std::string, RENDER_OUTPUT>::iterator i = render_outputs.begin(); i != render_outputs.end(); i++)
		{
			if (i->second.IsFBO())
				i->second.RenderToFBO().DeInit();
		}
		render_outputs.clear();
		texture_inputs.clear();
		
		OPENGL_UTILITY::CheckForOpenGLErrors("EnableShaders: FBO deinit", error_output);
		
		bool edge_contrast_enhancement = (lighting == 1);
		bool reflection_dynamic = (reflection_status == REFLECTION_DYNAMIC);
		bool shadows_near = shadows;
		bool shadows_medium = shadows && shadow_distance > 0;
		bool shadows_far = shadows && shadow_distance > 1;
		
		conditions.clear();
		#define ADDCONDITION(x) if (x) conditions.insert(#x)
		ADDCONDITION(bloom);
		ADDCONDITION(edge_contrast_enhancement);
		ADDCONDITION(reflection_dynamic);
		ADDCONDITION(shadows_near);
		ADDCONDITION(shadows_medium);
		ADDCONDITION(shadows_far);
		#undef ADDCONDITION
		
		for (std::vector <GRAPHICS_CONFIG_OUTPUT>::const_iterator i = config.outputs.begin(); i != config.outputs.end(); i++)
		{
			if (i->conditions.Satisfied(conditions))
			{
				assert(render_outputs.find(i->name) == render_outputs.end());
				
				if (i->type == "framebuffer")
				{
					render_outputs[i->name].RenderToFramebuffer();
				}
				else
				{
					FBTEXTURE & fbtex = render_outputs[i->name].RenderToFBO();
					FBTEXTURE::TARGET type = FBTEXTURE::NORMAL;
					if (i->type == "rectangle")
						type = FBTEXTURE::RECTANGLE;
					else if (i->type == "cube")
						type = FBTEXTURE::CUBEMAP;
					int fbms = 0;
					if (i->multisample < 0)
						fbms = fsaa;
					
					fbtex.Init(i->width.GetSize(w),
							   i->height.GetSize(h),
							   type,
							   (i->format == "depth"),
							   (i->filter == "nearest"),
							   (i->format == "RGBA"),
							   i->mipmap,
							   error_output,
							   fbms);
							   
					texture_inputs[i->name] = fbtex;
				}
				
				info_output << "Initialized render output: " << i->name << (i->type != "framebuffer" ? " (FBO)" : " (framebuffer alias)") << std::endl;
			}
		}

		render_outputs["framebuffer"].RenderToFramebuffer();
	}
	else
	{
		error_output << "Disabling shaders due to shader loading error" << endl;
		DisableShaders();
	}
}

bool GRAPHICS_SDLGL::ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output)
{
	EnableShaders(shaderpath, info_output, error_output);
	
	return GetUsingShaders();
}

void GRAPHICS_SDLGL::DisableShaders()
{
	shadermap.clear();
	using_shaders = false;

	if (GLEW_ARB_shading_language_100)
	{
		glUseProgramObjectARB(0);
	}
}

void GRAPHICS_SDLGL::EndScene(std::ostream & error_output)
{
	OPENGL_UTILITY::CheckForOpenGLErrors("EndScene", error_output);

	SDL_GL_SwapBuffers();

	OPENGL_UTILITY::CheckForOpenGLErrors("SDL_GL buffer swap", error_output);
}

void GRAPHICS_SDLGL::SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
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
		GRAPHICS_CAMERA & cam = cameras["dynamic reflection"];
		cam.pos = dynamic_reflection_sample_pos;
		cam.fov = 90;
		cam.orient.LoadIdentity();
		cam.view_distance = 100.f;
		cam.w = 1.f;
		cam.h = 1.f;
	}
	
	// create cameras for shadow passes
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
		glActiveTextureARB(GL_TEXTURE4_ARB+i);
		glMatrixMode( GL_TEXTURE );
		glLoadMatrixf(clipmat);
		glMatrixMode( GL_MODELVIEW );
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
}

bool SortDraworder(DRAWABLE * d1, DRAWABLE * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

std::string BuildKey(const std::string & camera, const std::string & draw)
{
	return camera + ";" + draw;
}

void GRAPHICS_SDLGL::DrawScene(std::ostream & error_output)
{
	renderscene.SetFlags(using_shaders);
	renderscene.SetFSAA(fsaa);

	if (reflection_status == REFLECTION_STATIC)
		renderscene.SetReflection(&static_reflection);
	renderscene.SetAmbient(static_ambient);
	renderscene.SetContrast(contrast);

	//sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);
	
	//shader path
	if (using_shaders)
	{
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
					std::string key = BuildKey(i->camera, *d);
					if (i->cull)
					{
						camera_map_type::iterator ci = cameras.find(i->camera);
						assert(ci != cameras.end()); //TODO: replace with friendly error message
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
							assert(container); //TODO: replace with friendly error message
							container->Query(frustum, culled_static_drawlist[key]);
							renderscene.DisableOrtho();
						}
					}
					else
					{
						reseatable_reference <AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> > container =
							static_drawlist.GetDrawlist().GetByName(*d);
						assert(container); //TODO: replace with friendly error message
						container->Query(AABB<float>::INTERSECT_ALWAYS(), culled_static_drawlist[key]);
					}
				}
			}
		}
		
		//construct light position
		MATHVECTOR <float, 3> lightposition(0,0,1);
		(-lightdirection).RotateVector(lightposition);
		renderscene.SetSunDirection(lightposition);
		
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
					RenderPostProcess(i->shader, input_textures, render_outputs[i->output], error_output);
				}
				else
				{
					for (std::vector <std::string>::const_iterator d = i->draw.begin(); d != i->draw.end(); d++)
					{
						camera_map_type::iterator ci = cameras.find(i->camera);
						assert(ci != cameras.end()); //TODO: replace with friendly error message
						GRAPHICS_CAMERA & cam = ci->second;
						if (cam.orthomode)
							renderscene.SetOrtho(cam.orthomin, cam.orthomax);
						else
							renderscene.DisableOrtho();
						renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
						
						shader_map_type::iterator si = shadermap.find(i->shader);
						assert(si != shadermap.end()); //TODO: replace with friendly error message
						renderscene.SetDefaultShader(si->second);
						
						if (d == i->draw.begin())
							renderscene.SetClear(i->clear_color, i->clear_depth);
						else
							renderscene.SetClear(false, false);
						renderscene.SetWriteDepth(i->write_depth);
						
						reseatable_reference <PTRVECTOR <DRAWABLE> > container = dynamic_drawlist.GetByName(*d);
						assert(container); //TODO: replace with friendly error message
						
						render_output_map_type::iterator oi = render_outputs.find(i->output);
						assert(oi != render_outputs.end()); //TODO: replace with friendly error message
						
						RenderDrawlists(*container,
											culled_static_drawlist[BuildKey(i->camera,*d)],
											input_textures,
											renderscene,
											oi->second,
											error_output);
											
						renderscene.DisableOrtho();
					}
				}
			}
		}
		
		/*//shadow pre-passes
		if (shadows)
		{
			DRAWABLE_CONTAINER <PTRVECTOR> specialcam_static_drawlist;
			
			int csm_count = 0;
			std::vector <MATRIX4 <float> > shadow_clipmat(shadow_depthtexturelist.size()); //hold the clip matrices for the shadow maps so we can send to the "full" shader later
			for (std::list <RENDER_OUTPUT>::iterator i = shadow_depthtexturelist.begin(); i != shadow_depthtexturelist.end(); ++i,++csm_count)
			{
				//determine the correct camera position to emulate the sun

				//float closeshadow = 5.0;
				//if (game.world.GetCameraMode() == CMInCar)
				//	closeshadow = 2.0;
				float shadow_radius = (1<<csm_count)*closeshadow+(csm_count)*20.0; //5,30,60

				//float lambda = 0.75;
				//float nd = 0.1;
				//float fd = 30.0;
				//float si = (csm_count+1.0)/shadow_depthtexturelist.size();
				//float shadow_radius = lambda*(nd*powf(fd/nd,si))+(1.0-lambda)*(nd+(fd-nd)*si);

				// setup the camera
				MATHVECTOR <float, 3> shadowbox(1,1,1);
				shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
				MATHVECTOR <float, 3> shadowoffset(0,0,-1);
				shadowoffset = shadowoffset * shadow_radius;
				(-camorient).RotateVector(shadowoffset);
				//if (csm_count == 1) std::cout << shadowoffset << std::endl;
				shadowbox[2] += 60.0;
				renderscene.SetOrtho(-shadowbox, shadowbox);
				FRUSTUM frustum = renderscene.SetCameraInfo(campos+shadowoffset, lightdirection, camfov, 10000.0, w, h);
				
				// do fast culling
				specialcam_static_drawlist.clear();
				static_drawlist.GetDrawlist().normal_noblend.Query(frustum, specialcam_static_drawlist.normal_noblend);

				#ifdef _SHADOWMAP_DEBUG_
				renderscene.SetClear(true, true);
				#else
				renderscene.SetClear(false, true);
				#endif
				if (csm_count == 0 && shadow_quality == 4)
					renderscene.SetDefaultShader(shadermap["depthgen2"]);
				else
					renderscene.SetDefaultShader(shadermap["depthgen"]);
				
				RenderDrawlist(dynamic_drawlist.car_noblend, renderscene, *i, error_output);
				renderscene.SetClear(false,false);
				RenderDrawlists(dynamic_drawlist.normal_noblend, specialcam_static_drawlist.normal_noblend, renderscene, *i, error_output);
				
				//extract matrices for shadowing.  this is possible because the Render function for RENDER_INPUT_SCENE does not pop matrices at the end
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
				shadow_clipmat[csm_count].Set(clipmat);
				glPopMatrix();
				glMatrixMode( GL_MODELVIEW );
			}

			renderscene.DisableOrtho();

			//send the shadow projection matrices to the texture matrices and activate the CSMs
			csm_count = 0;
			for (std::list <RENDER_OUTPUT>::iterator i = shadow_depthtexturelist.begin(); i != shadow_depthtexturelist.end(); ++i,++csm_count)
			{
				glActiveTextureARB(GL_TEXTURE4_ARB+csm_count);
				glMatrixMode(GL_TEXTURE);
				glLoadMatrixf(shadow_clipmat[csm_count].GetArray());
				i->RenderToFBO().Activate();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
			}

			glMatrixMode(GL_MODELVIEW);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		//edge contrast enhancement pre-pass to generate depth map
		if (lighting == 1)
		{
			renderscene.SetDefaultShader(shadermap["depthonly"]);
			
			RENDER_OUTPUT & edgecontrastenhancement_depths = render_outputs["edgecontrastenhancement_depths"];
			
			renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes
			renderscene.SetClear(false, true);
			RenderDrawlists(dynamic_drawlist.skybox_noblend, normalcam_static_drawlist.skybox_noblend, renderscene, edgecontrastenhancement_depths, error_output);
			renderscene.SetClear(false, false);
			RenderDrawlists(dynamic_drawlist.skybox_blend, normalcam_static_drawlist.skybox_blend, renderscene, edgecontrastenhancement_depths, error_output);
			
			renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);
			RenderDrawlists(dynamic_drawlist.normal_noblend, normalcam_static_drawlist.normal_noblend, renderscene, edgecontrastenhancement_depths, error_output);
			RenderDrawlist(dynamic_drawlist.car_noblend, renderscene, edgecontrastenhancement_depths, error_output);
			
			//load the texture
			glActiveTexture(GL_TEXTURE9);
			glEnable(GL_TEXTURE_2D);
			edgecontrastenhancement_depths.RenderToFBO().Activate();
			glActiveTexture(GL_TEXTURE0);
		}
		
		//dynamic reflection cubemap pre-pass to generate reflection map
		if (reflection_status == REFLECTION_DYNAMIC)
		{
			OPENGL_UTILITY::CheckForOpenGLErrors("reflection map generation: begin", error_output);
			
			RENDER_OUTPUT & dynamic_reflection = render_outputs["dynamic_reflection"];
			FBTEXTURE & reflection_fbo = dynamic_reflection.RenderToFBO();
			
			const float pi = 3.141593;
			
			DRAWABLE_CONTAINER <PTRVECTOR> specialcam_static_drawlist;
			
			for (int i = 0; i < 6; i++)
			{
				QUATERNION <float> orient;
				
				// set up our target
				switch (i)
				{
					case 0:
					reflection_fbo.SetCubeSide(FBTEXTURE::POSX);
					orient.Rotate(pi*0.5, 0,1,0);
					break;
					
					case 1:
					reflection_fbo.SetCubeSide(FBTEXTURE::NEGX);
					orient.Rotate(-pi*0.5, 0,1,0);
					break;
					
					case 2:
					reflection_fbo.SetCubeSide(FBTEXTURE::POSY);
					orient.Rotate(pi*0.5, 1,0,0);
					break;
					
					case 3:
					reflection_fbo.SetCubeSide(FBTEXTURE::NEGY);
					orient.Rotate(-pi*0.5, 1,0,0);
					break;
					
					case 4:
					reflection_fbo.SetCubeSide(FBTEXTURE::POSZ);
					// orient is already set up for us!
					break;
					
					case 5:
					reflection_fbo.SetCubeSide(FBTEXTURE::NEGZ);
					orient.Rotate(pi, 0,1,0);
					break;
					
					default:
					error_output << "Reached odd spot while building dynamic reflection cubemap. How many sides are in a cube, anyway? " << i << "?" << std::endl;
					break;
				};
				
				OPENGL_UTILITY::CheckForOpenGLErrors("reflection map generation: FBO cube side attachment", error_output);
				
				float fov = 90;
				int rw = reflection_fbo.GetWidth();
				int rh = reflection_fbo.GetHeight();
				
				// make sure we don't ever try to bind ourselves
				renderscene.SetReflection(NULL);
				
				// render the scene
				renderscene.SetDefaultShader(shadermap["simple"]);
				renderscene.SetCameraInfo(dynamic_reflection_sample_position, orient, fov, 10000.0, rw, rh); //use very high draw distance for skyboxes
				renderscene.SetClear(true, true);
				RenderDrawlists(dynamic_drawlist.skybox_noblend, normalcam_static_drawlist.skybox_noblend, renderscene, dynamic_reflection, error_output);
				renderscene.SetClear(false, false);
				RenderDrawlists(dynamic_drawlist.skybox_blend, normalcam_static_drawlist.skybox_blend, renderscene, dynamic_reflection, error_output);
				
				// do fast culling
				FRUSTUM frustum = renderscene.SetCameraInfo(dynamic_reflection_sample_position, orient, fov, 100.0, rw, rh); //use a smaller draw distance than normal
				specialcam_static_drawlist.clear();
				static_drawlist.GetDrawlist().normal_noblend.Query(frustum, specialcam_static_drawlist.normal_noblend);
				
				// continue rendering
				RenderDrawlists(dynamic_drawlist.normal_noblend, specialcam_static_drawlist.normal_noblend, renderscene, dynamic_reflection, error_output);
			}
			
			OPENGL_UTILITY::CheckForOpenGLErrors("reflection map generation: end", error_output);
			
			// set it up to be used
			renderscene.SetReflection(&reflection_fbo);
		}

		renderscene.SetSunDirection(lightposition);
		
		renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes

		//determine render output for the full scene
		reseatable_reference <RENDER_OUTPUT> scenebuffer = render_outputs["framebuffer"];
		if (bloom)
			scenebuffer = render_outputs["full_scene_buffer"];

		renderscene.SetClear(true, true);
		renderscene.SetDefaultShader(shadermap["skybox"]);
		RenderDrawlists(dynamic_drawlist.skybox_noblend, normalcam_static_drawlist.skybox_noblend, renderscene, *scenebuffer, error_output);
		renderscene.SetClear(false, true);
		RenderDrawlists(dynamic_drawlist.skybox_blend, normalcam_static_drawlist.skybox_blend, renderscene, *scenebuffer, error_output);

		
		// render normal opaque geometry using a z-prepass to avoid costly pixel computations for overdrawn geometry
		const bool enable_z_prepass = false;
		
		// z only prepass
		renderscene.SetClear(false, true);
		if (enable_z_prepass)
		{
			renderscene.SetWriteColor(false);
			renderscene.SetDefaultShader(shadermap["depthgen"]);
			RenderDrawlists(dynamic_drawlist.normal_noblend, normalcam_static_drawlist.normal_noblend, renderscene, *scenebuffer, error_output);
			renderscene.SetClear(false, false);
			RenderDrawlist(dynamic_drawlist.car_noblend, renderscene, *scenebuffer, error_output);
			renderscene.SetWriteColor(true);
		}
		
		// color pass
		renderscene.SetDefaultShader(shadermap["full"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_DISTANCEFIELD, shadermap["distancefield"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_SIMPLE, shadermap["simple"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_SKYBOX, shadermap["skybox"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_FULL, shadermap["full_noblend"]);
		RenderDrawlists(dynamic_drawlist.normal_noblend, normalcam_static_drawlist.normal_noblend, renderscene, *scenebuffer, error_output);
		renderscene.SetClear(false,false);
		RenderDrawlist(dynamic_drawlist.car_noblend, renderscene, *scenebuffer, error_output);
		
		
		RenderDrawlists(dynamic_drawlist.normal_blend, normalcam_static_drawlist.normal_blend, renderscene, *scenebuffer, error_output);
		renderscene.SetWriteDepth(false);
		RenderDrawlist(dynamic_drawlist.particle, renderscene, *scenebuffer, error_output);
		renderscene.SetWriteDepth(true);
		
		if (bloom) //do bloom post-processing
		{
			RENDER_OUTPUT & full_scene_buffer = render_outputs["full_scene_buffer"];
			RENDER_OUTPUT & bloom_buffer = render_outputs["bloom_buffer"];
			RENDER_OUTPUT & blur_buffer = render_outputs["blur_buffer"];
			
			//create a map of highlights
			RenderPostProcess("bloompass", full_scene_buffer.RenderToFBO(), bloom_buffer, error_output);

			//blur horizontally
			RenderPostProcess("gaussian_blur_horizontal", bloom_buffer.RenderToFBO(), blur_buffer, error_output);

			//blur vertically and ping-pong back to the bloom buffer
			RenderPostProcess("gaussian_blur_vertical", blur_buffer.RenderToFBO(), bloom_buffer, error_output);
			
			//composite the final results
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			bloom_buffer.RenderToFBO().Activate();
			glActiveTexture(GL_TEXTURE0);
			RenderPostProcess("bloomcomposite", full_scene_buffer.RenderToFBO(), render_outputs["framebuffer"], error_output);
		}
		
		RENDER_OUTPUT & final = render_outputs["framebuffer"];

		//debug shadow texture drawing
		#ifdef _SHADOWMAP_DEBUG_
		RENDER_INPUT_POSTPROCESS depth_lookup;
		assert(!shadow_depthtexturelist.empty());
		depth_lookup.SetShader(&shadermap["simple"]);
		depth_lookup.SetSourceTexture(shadow_depthtexturelist.back().RenderToFBO());
		Render(&depth_lookup, final, error_output);
		#endif
		
		// debugging for dynamic reflections
		#ifdef _DYNAMIC_REFLECT_DEBUG_
		if (reflection_status == REFLECTION_DYNAMIC)
		{
			FBTEXTURE & reflection_fbo = dynamic_reflection.RenderToFBO();
			RENDER_INPUT_POSTPROCESS cubelookup;
			cubelookup.SetShader(&shadermap["simplecube"]);
			cubelookup.SetSourceTexture(reflection_fbo);
			Render(&cubelookup, final, error_output);
		}
		#endif

		RenderDrawlist(dynamic_drawlist.twodim, renderscene, final, error_output);
		RenderDrawlist(dynamic_drawlist.text, renderscene, final, error_output);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, 45.0, view_distance, w, h);
		RenderDrawlist(dynamic_drawlist.nocamtrans_noblend, renderscene, final, error_output);
		renderscene.SetClear(false, false);
		RenderDrawlist(dynamic_drawlist.nocamtrans_blend, renderscene, final, error_output);*/
	}
	else //non-shader path
	{
		GRAPHICS_CAMERA & cam = cameras["default"];
		renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
		
		//do fast culling for the normal camera frustum
		FRUSTUM normalcam = renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
		DRAWABLE_CONTAINER <PTRVECTOR> normalcam_static_drawlist;
		static_drawlist.GetDrawlist().normal_noblend.Query(normalcam, normalcam_static_drawlist.normal_noblend);
		//std::cout << "Fast cull from " << static_drawlist.GetDrawlist().normal_noblend.size() << " to " << normalcam_static_drawlist.normal_noblend.size() << std::endl;
		static_drawlist.GetDrawlist().normal_blend.Query(normalcam, normalcam_static_drawlist.normal_blend);
		//note that all skybox objects go in, no frustum culling for them
		static_drawlist.GetDrawlist().skybox_blend.Query(AABB<float>::INTERSECT_ALWAYS(), normalcam_static_drawlist.skybox_blend);
		static_drawlist.GetDrawlist().skybox_noblend.Query(AABB<float>::INTERSECT_ALWAYS(), normalcam_static_drawlist.skybox_noblend);
		
		RENDER_OUTPUT framebuffer;
		framebuffer.RenderToFramebuffer();
		
		// render skybox
		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, 10000.0, cam.w, cam.h); //use very high draw distance for skyboxes
		RenderDrawlists(dynamic_drawlist.skybox_noblend, normalcam_static_drawlist.skybox_noblend, std::vector <TEXTURE_INTERFACE*>(), renderscene, framebuffer, error_output);
		renderscene.SetClear(false, true);
		RenderDrawlists(dynamic_drawlist.skybox_blend, normalcam_static_drawlist.skybox_blend, std::vector <TEXTURE_INTERFACE*>(), renderscene, framebuffer, error_output);

		// render most 3d stuff
		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(cam.pos, cam.orient, cam.fov, cam.view_distance, cam.w, cam.h);
		RenderDrawlists(dynamic_drawlist.normal_noblend, normalcam_static_drawlist.normal_noblend, std::vector <TEXTURE_INTERFACE*>(), renderscene, framebuffer, error_output);
		renderscene.SetClear(false, false);
		
		renderscene.SetCarPaintHack(true);
		RenderDrawlist(dynamic_drawlist.car_noblend, renderscene, framebuffer, error_output);
		renderscene.SetCarPaintHack(false);
		
		RenderDrawlists(dynamic_drawlist.normal_blend, normalcam_static_drawlist.normal_blend, std::vector <TEXTURE_INTERFACE*>(), renderscene, framebuffer, error_output);
		RenderDrawlist(dynamic_drawlist.particle, renderscene, framebuffer, error_output);
		RenderDrawlist(dynamic_drawlist.twodim, renderscene, framebuffer, error_output);
		RenderDrawlist(dynamic_drawlist.text, renderscene, framebuffer, error_output);

		// render any viewspace 3d elements
		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(cam.pos, cam.orient, 45.0, cam.view_distance, cam.w, cam.h);
		renderscene.SetCarPaintHack(true);
		RenderDrawlist(dynamic_drawlist.nocamtrans_noblend, renderscene, framebuffer, error_output);
		renderscene.SetCarPaintHack(false);
		renderscene.SetClear(false, false);
		RenderDrawlist(dynamic_drawlist.nocamtrans_blend, renderscene, framebuffer, error_output);
	}
}

void GRAPHICS_SDLGL::RenderDrawlist(std::vector <DRAWABLE*> & drawlist,
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

void GRAPHICS_SDLGL::RenderDrawlists(std::vector <DRAWABLE*> & dynamic_drawlist,
						std::vector <DRAWABLE*> & static_drawlist,
						const std::vector <TEXTURE_INTERFACE*> & extra_textures,
						RENDER_INPUT_SCENE & render_scene, 
						RENDER_OUTPUT & render_output, 
						std::ostream & error_output)
{
	if (dynamic_drawlist.empty() && static_drawlist.empty() && !render_scene.GetClear().first && !render_scene.GetClear().second)
		return;
	
	for (unsigned int i = 0; i < extra_textures.size(); i++)
	{
		if (extra_textures[i])
			glstate.BindTexture2D(i, extra_textures[i]);
	}
	
	render_scene.SetDrawLists(dynamic_drawlist, static_drawlist);
	Render(&render_scene, render_output, error_output);
}

void GRAPHICS_SDLGL::RenderPostProcess(const std::string & shadername,
						const std::vector <TEXTURE_INTERFACE*> & textures,
						RENDER_OUTPUT & render_output, 
						std::ostream & error_output)
{
	RENDER_INPUT_POSTPROCESS postprocess;
	std::map <std::string, SHADER_GLSL>::iterator s = shadermap.find(shadername);
	assert(s != shadermap.end());
	postprocess.SetShader(&s->second);
	postprocess.SetSourceTextures(textures);
	Render(&postprocess, render_output, error_output);
}

void GRAPHICS_SDLGL::DrawBox(const MATHVECTOR <float, 3> & corner1, const MATHVECTOR <float, 3> & corner2) const
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

void GRAPHICS_SDLGL::Render(RENDER_INPUT * input, RENDER_OUTPUT & output, std::ostream & error_output)
{
	output.Begin(glstate, error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output begin", error_output);

	input->Render(glstate, error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render finish", error_output);

	output.End(error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output end", error_output);
}

void GRAPHICS_SDLGL::AddStaticNode(SCENENODE & node, bool clearcurrent)
{
	static_drawlist.Generate(node, clearcurrent);
}

void GRAPHICS_SDLGL::Screenshot(std::string filename)
{
	SDL_Surface *screen;
	SDL_Surface *temp = NULL;
	unsigned char *pixels;
	int i;

	screen = surface;

	if (!(screen->flags & SDL_OPENGL))
	{
		SDL_SaveBMP(temp, filename.c_str());
		return;
	}

	temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
		0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
		);

	assert(temp);

	pixels = (unsigned char *) malloc(3 * screen->w * screen->h);
	assert(pixels);

	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	for (i=0; i<screen->h; i++)
		memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3*screen->w * (screen->h-i-1), screen->w*3);
	free(pixels);

	SDL_SaveBMP(temp, filename.c_str());
	SDL_FreeSurface(temp);
}

bool TextureSort(const DRAWABLE & draw1, const DRAWABLE & draw2)
{
	return (draw1.GetDiffuseMap()->GetID() < draw2.GetDiffuseMap()->GetID());
}

/*void GRAPHICS_SDLGL::OptimizeStaticDrawlistmap()
{
	static_object_partitioning.clear();
	
	for (std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> >::iterator i = static_drawlist_map.begin(); i != static_drawlist_map.end(); ++i)
	{
		AABB_SPACE_PARTITIONING_NODE <SCENEDRAW*> & partition = static_object_partitioning[i->first];
		
		for (std::vector <SCENEDRAW>::iterator s = i->second.begin(); s != i->second.end(); s++)
		{
			const DRAWABLE * draw = s->GetDraw();
			assert(draw);
			MATHVECTOR <float, 3> objpos(draw->GetObjectCenter());
			if (s->IsCollapsed())
				s->GetMatrix4()->TransformVectorOut(objpos[0],objpos[1],objpos[2]);
			else if (draw->GetParent() != NULL)
				objpos = draw->GetParent()->TransformIntoWorldSpace(objpos);
			float radius = draw->GetRadius()*1.732051;
			AABB <float> bbox;
			bbox.SetFromCorners(objpos-MATHVECTOR<float,3>(radius), objpos+MATHVECTOR<float,3>(radius));
			SCENEDRAW * newsd = &(*s);
			partition.Add(newsd, bbox);
		}
		
		partition.Optimize();
		
		//std::sort(i->second.begin(), i->second.end(), TextureSort);
	}
	//TODO: update/remove OptimizeStaticDrawlistmap
}*/
