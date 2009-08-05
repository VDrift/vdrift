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
#include "scenegraph.h"
#include "matrix4.h"
#include "mathvector.h"
#include "model.h"
#include "texture.h"
#include "vertexarray.h"
#include "reseatable_reference.h"
#include "definitions.h"
#include "containeralgorithm.h"

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
#include <numeric>

//#define _SHADOWMAP_DEBUG_

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

	if (reflection_type > 0)
		reflection_status = REFLECTION_STATIC;

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

	if (!GLEW_ARB_texture_cube_map)
	{
		info_output << "Your video card doesn't support cube maps.  Disabling shaders." << endl;
		DisableShaders();
	}

	if (GLEW_ARB_shading_language_100 && GLEW_VERSION_2_0 && shaders && GLEW_ARB_fragment_shader)
		EnableShaders(shaderpath, info_output, error_output);
	else
	{
		info_output << "Disabling shaders" << endl;
		DisableShaders();
	}

	if (GLEW_EXT_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);

	info_output << "Maximum anisotropy: " << max_anisotropy << endl;

	OPENGL_UTILITY::CheckForOpenGLErrors("Shader loading", error_output);

	//load reflection map
	if (reflection_status == REFLECTION_STATIC && !static_reflectionmap_file.empty())
	{
		TEXTUREINFO t;
		t.SetName(static_reflectionmap_file);
		t.SetCube(true, true);
		t.SetMipMap(false);
		t.SetAnisotropy(anisotropy);
		static_reflection.Load(t, error_output, texturesize);
	}
	
	if (!static_ambientmap_file.empty())
	{
		TEXTUREINFO t;
		t.SetName(static_ambientmap_file);
		t.SetCube(true, true);
		t.SetMipMap(false);
		t.SetAnisotropy(anisotropy);
		static_ambient.Load(t, error_output, texturesize);
	}

	//initialize scenegraph structures
	no2d_noblend.SetFilter_is2d(true, false);
	no2d_noblend.SetFilter_partial_transparency(true, false);
	no2d_noblend.SetFilter_skybox(true,false);
	no2d_noblend.SetFilter_cameratransform(true, true);

	no2d_blend.SetFilter_is2d(true, false);
	no2d_blend.SetFilter_partial_transparency(true, true);
	no2d_blend.SetFilter_skybox(true,false);
	no2d_blend.SetFilter_cameratransform(true, true);

	only2d.SetFilter_is2d(true, true);
	only2d.SetFilter_skybox(true, false);

	skyboxes_noblend.SetFilter_skybox(true,true);
	skyboxes_noblend.SetFilter_cameratransform(true, true);
	skyboxes_noblend.SetFilter_partial_transparency(true, false);

	skyboxes_blend.SetFilter_skybox(true,true);
	skyboxes_blend.SetFilter_cameratransform(true, true);
	skyboxes_blend.SetFilter_partial_transparency(true, true);

	camtransfilter.SetFilter_cameratransform(true, false);

	filter_list.push_back(&no2d_noblend);
	filter_list.push_back(&no2d_blend);
	filter_list.push_back(&only2d);
	filter_list.push_back(&skyboxes_blend);
	filter_list.push_back(&skyboxes_noblend);
	filter_list.push_back(&camtransfilter);

	for (std::list <DRAWABLE_FILTER *>::iterator i = filter_list.begin(); i != filter_list.end(); ++i)
	{
		static_drawlist_map[*i];
		dynamic_drawlist_map[*i];
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

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SetActiveShader(const SHADER_TYPE & newshader)
{
	if (newshader == activeshader)
		return;

	assert(GLEW_ARB_shading_language_100);

	size_t shaderindex = newshader;

	assert(shaderindex < shadermap.size());
	assert(shadermap[shaderindex] != NULL);

	shadermap[shaderindex]->Enable();
	activeshader = newshader;
}

///note that if variant is passed in, it is appended to the shader name and the shader is also loaded with the variant_defines set
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
		shadername = name + variant;
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
			info_output << "Loaded shader package "+name << endl;
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

	shader_load_success = shader_load_success && LoadShader(shaderpath, "simple", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "less_simple", info_output, error_output);
	//shader_load_success = shader_load_success && LoadShader(shaderpath, "full", info_output, error_output, "_noblend", "_ALPHATEST_");
	shader_load_success = shader_load_success && LoadShader(shaderpath, "full", info_output, error_output, "_noblend", "_ALPHATEST_");
	shader_load_success = shader_load_success && LoadShader(shaderpath, "full", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "depthgen", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "depthgen2", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "depthonly", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "distancefield", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "bloompass", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "bloomcomposite", info_output, error_output);
	shader_load_success = shader_load_success && LoadShader(shaderpath, "gaussian_blur", info_output, error_output, "_horizontal", "_HORIZONTAL_");
	shader_load_success = shader_load_success && LoadShader(shaderpath, "gaussian_blur", info_output, error_output, "_vertical", "_VERTICAL_");

	if (shader_load_success)
	{
		using_shaders = true;
		info_output << "Successfully enabled shaders" << endl;

		//set up the rendering pipeline
		shadow_depthtexturelist.clear();
		if (shadows)
		{
			for (int i = 0; i < shadow_distance+1; i++)
				shadow_depthtexturelist.push_back(RENDER_OUTPUT());
			int shadow_resolution = 512;
			if (shadow_quality == 1)
				shadow_resolution = 1024;
			if (shadow_quality >= 2)
				shadow_resolution = 2048;
			int count = 0;
			for (std::list <RENDER_OUTPUT>::iterator i = shadow_depthtexturelist.begin(); i != shadow_depthtexturelist.end(); ++i)
			{
				FBTEXTURE_GL & depthFBO = i->RenderToFBO();
#ifdef _SHADOWMAP_DEBUG_
				depthFBO.Init(shadow_resolution, shadow_resolution, false, false, false, false, error_output);
#else
				if (shadow_quality < 4 || count > 0)
					depthFBO.Init(shadow_resolution, shadow_resolution, false, true, false, false, error_output); //depth texture
				else
					depthFBO.Init(shadow_resolution, shadow_resolution, false, false, true, false, error_output); //RGBA texture w/ nearest neighbor filtering
#endif
				count++;
			}
		}

		if (lighting == 1)
		{
			int map_size = 1024;
			FBTEXTURE_GL & depthFBO = edgecontrastenhancement_depths.RenderToFBO();
			depthFBO.Init(map_size, map_size, false, true, false, false, error_output);
		}

		if (bloom) //generate a screen-sized rectangular FBO
		{
			FBTEXTURE_GL & sceneFBO = full_scene_buffer.RenderToFBO();
			sceneFBO.Init(w, h, true, false, false, true, error_output);
		}

		if (bloom) //generate a buffer to store the bloom result plus a buffer for separable blur intermediate step
		{
			int bloom_size = 512;
			FBTEXTURE_GL & bloomFBO = bloom_buffer.RenderToFBO();
			bloomFBO.Init(bloom_size, bloom_size, false, false, false, false, error_output);

			FBTEXTURE_GL & blurFBO = blur_buffer.RenderToFBO();
			blurFBO.Init(bloom_size, bloom_size, false, false, false, false, error_output);
		}

		final.RenderToFramebuffer();
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

void GRAPHICS_SDLGL::SetupCamera()
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	//gluPerspective( camfov, w/(float)h, 0.1f, 10000.0f );
	gluPerspective( camfov, w/(float)h, 0.1f, 10000.0f );
	glMatrixMode( GL_MODELVIEW );
	float temp_matrix[16];
	(camorient).GetMatrix4(temp_matrix);
	glLoadMatrixf(temp_matrix);
	glTranslatef(-campos[0],-campos[1],-campos[2]);
	//game.cam.ExtractFrustum();
}

/*void GRAPHICS_SDLGL::GenerateCombinedDrawlistMap()
{
	for (std::map <DRAWABLE_FILTER *, std::vector <SCENEDRAW> >::iterator i = combined_drawlist_map.begin(); i != combined_drawlist_map.end(); i++)
	{
		i->second.resize(0);
		std::vector <SCENEDRAW> & staticdraw = static_drawlist_map[i->first];
		std::vector <SCENEDRAW> & dynamicdraw = dynamic_drawlist_map[i->first];
		calgo::copy(staticdraw,std::back_inserter(i->second));
		calgo::copy(dynamicdraw,std::back_inserter(i->second));
	}
}*/

void GRAPHICS_SDLGL::SendDrawlistToRenderScene(RENDER_INPUT_SCENE & renderscene, DRAWABLE_FILTER * filter_ptr)
{
	bool speedup = false;
	if (filter_ptr->Allows2D() || filter_ptr->AllowsNoCameraTransform() || filter_ptr->AllowsSkybox())
		speedup = false;
	renderscene.SetDrawList(static_drawlist_map[filter_ptr], dynamic_drawlist_map[filter_ptr], static_object_partitioning[filter_ptr], speedup);
}

void GRAPHICS_SDLGL::DrawScene(std::ostream & error_output)
{
	renderscene.SetFlags(using_shaders);
	renderscene.SetFSAA(fsaa);
	renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);

	if (reflection_status == REFLECTION_STATIC)
		renderscene.SetReflection(static_reflection);
	renderscene.SetAmbient(static_ambient);
	renderscene.SetContrast(contrast);

	/*std::cout << "no2d_noblend: " << drawlist_map[&no2d_noblend].size() << std::endl;
	std::cout << "no2d_blend: " << drawlist_map[&no2d_blend].size() << std::endl;
	std::cout << "only2d: " << drawlist_map[&only2d].size() << std::endl;
	std::cout << "skyboxes_blend: " << drawlist_map[&skyboxes_blend].size() << std::endl;
	std::cout << "skyboxes_noblend: " << drawlist_map[&skyboxes_noblend].size() << std::endl;
	std::cout << "camtransfilter: " << drawlist_map[&camtransfilter].size() << std::endl;
	std::cout << std::endl;*/

	//shader path
	if (using_shaders)
	{
		//construct light position
		MATHVECTOR <float, 3> lightposition(0,0,1);
		(-lightdirection).RotateVector(lightposition);

		//shadow pre-passes
		if (shadows)
		{
			int csm_count = 0;
			std::vector <MATRIX4 <float> > shadow_clipmat(shadow_depthtexturelist.size()); //hold the clip matrices for the shadow maps so we can send to the "full" shader later
			for (std::list <RENDER_OUTPUT>::iterator i = shadow_depthtexturelist.begin(); i != shadow_depthtexturelist.end(); ++i,++csm_count)
			{
				//determine the correct camera position to emulate the sun

				//float closeshadow = 5.0;
				//if (game.world.GetCameraMode() == CMInCar)
				//	closeshadow = 2.0;
				float shadow_radius = (1<<csm_count)*closeshadow+(csm_count)*20.0; //5,30,60

				/*float lambda = 0.75;
				float nd = 0.1;
				float fd = 30.0;
				float si = (csm_count+1.0)/shadow_depthtexturelist.size();
				float shadow_radius = lambda*(nd*powf(fd/nd,si))+(1.0-lambda)*(nd+(fd-nd)*si);*/

				MATHVECTOR <float, 3> shadowbox(1,1,1);
				shadowbox = shadowbox * (shadow_radius*sqrt(2));
				MATHVECTOR <float, 3> shadowoffset(0,0,-1);
				shadowoffset = shadowoffset * shadow_radius;
				(-camorient).RotateVector(shadowoffset);
				//if (csm_count == 1) std::cout << shadowoffset << std::endl;
				shadowbox[2] += 60.0;
				renderscene.SetOrtho(-shadowbox, shadowbox);
				renderscene.SetCameraInfo(campos+shadowoffset, lightdirection, camfov, 10000.0, w, h);

				#ifdef _SHADOWMAP_DEBUG_
				renderscene.SetClear(true, true);
				#else
				renderscene.SetClear(false, true);
				#endif
				if (csm_count == 0 && shadow_quality == 4)
					renderscene.SetDefaultShader(shadermap["depthgen2"]);
				else
					renderscene.SetDefaultShader(shadermap["depthgen"]);
				SendDrawlistToRenderScene(renderscene,&no2d_noblend);
				Render(&renderscene, *i, error_output);
				//renderscene.SetClear(false, false);
				//SendDrawlistToRenderScene(renderscene,&no2d_blend);
				//Render(&renderscene, *i, error_output);

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
			renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes
			renderscene.SetClear(false, true);
			SendDrawlistToRenderScene(renderscene,&skyboxes_noblend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			renderscene.SetClear(false, false);
			SendDrawlistToRenderScene(renderscene,&skyboxes_blend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);
			SendDrawlistToRenderScene(renderscene,&no2d_noblend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			//SendDrawlistToRenderScene(renderscene,&no2d_blend);
			//Render(&renderscene, edgecontrastenhancement_depths, error_output);

			//load the texture
			glActiveTexture(GL_TEXTURE7);
			glEnable(GL_TEXTURE_2D);
			edgecontrastenhancement_depths.RenderToFBO().Activate();
			glActiveTexture(GL_TEXTURE0);
		}

		renderscene.SetSunDirection(lightposition);
		renderscene.SetDefaultShader(shadermap["full"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_DISTANCEFIELD, shadermap["distancefield"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_SIMPLE, shadermap["simple"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_SKYBOX, shadermap["less_simple"]);
		renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_FULL, shadermap["full_noblend"]);
		//renderscene.SetShader(RENDER_INPUT_SCENE::SHADER_FULLBLEND, shadermap["full"]);

		renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes
		//std::reverse(drawlist_map[&skyboxes].begin(), drawlist_map[&skyboxes].end());

		renderscene.SetClear(true, true);

		//determine render output for the full scene
		reseatable_reference <RENDER_OUTPUT> scenebuffer = final;
		if (bloom)
			scenebuffer = full_scene_buffer;

		SendDrawlistToRenderScene(renderscene,&skyboxes_noblend);
		Render(&renderscene, *scenebuffer, error_output);
		renderscene.SetClear(false, true);
		SendDrawlistToRenderScene(renderscene,&skyboxes_blend);
		//std::reverse(drawlist_map[&skyboxes].begin(), drawlist_map[&skyboxes].end());
		Render(&renderscene, *scenebuffer, error_output);
		renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);

		//debug shadow camera positioning
		/*int csm_count = 0;
		float closeshadow = 5.0;
		float shadow_radius = (1<<csm_count)*closeshadow+(csm_count)*20.0; //5,30
		MATHVECTOR <float, 3> shadowbox(1,1,1);
		shadowbox = shadowbox * (shadow_radius*sqrt(2));
		MATHVECTOR <float, 3> shadowoffset(0,0,1);
		shadowoffset = shadowoffset * shadow_radius;
		camorient.RotateVector(shadowoffset);
		MATHVECTOR <float, 3> orthomin = -shadowbox;
		orthomin[2] -= 20.0;
		renderscene.SetOrtho(orthomin, shadowbox);
		renderscene.SetCameraInfo(campos+shadowoffset, ldir, camfov, 10000.0, w, h);*/

		renderscene.SetClear(false, true);
		SendDrawlistToRenderScene(renderscene,&no2d_noblend);
		Render(&renderscene, *scenebuffer, error_output);
		
		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,&no2d_blend);
		Render(&renderscene, *scenebuffer, error_output);

		if (bloom) //do bloom post-processing
		{
			RENDER_INPUT_POSTPROCESS bloom_postprocess;

			//create a map of highlights
			bloom_postprocess.SetShader(&shadermap["bloompass"]);
			bloom_postprocess.SetSourceTexture(full_scene_buffer.RenderToFBO());
			//Render(&bloom_postprocess, final, error_output); //testing bloom processing
			Render(&bloom_postprocess, bloom_buffer, error_output);

			//blur horizontally
			bloom_postprocess.SetShader(&shadermap["gaussian_blur_horizontal"]);
			bloom_postprocess.SetSourceTexture(bloom_buffer.RenderToFBO());
			Render(&bloom_postprocess, blur_buffer, error_output);

			//blur vertically and ping-pong back to the bloom buffer
			bloom_postprocess.SetShader(&shadermap["gaussian_blur_vertical"]);
			bloom_postprocess.SetSourceTexture(blur_buffer.RenderToFBO());
			Render(&bloom_postprocess, bloom_buffer, error_output);

			//composite the final results
			bloom_postprocess.SetShader(&shadermap["bloomcomposite"]);
			bloom_postprocess.SetSourceTexture(full_scene_buffer.RenderToFBO());
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_2D);
			bloom_buffer.RenderToFBO().Activate();
			glActiveTexture(GL_TEXTURE0);
			Render(&bloom_postprocess, final, error_output);
		}

		//debug shadow texture drawing
		#ifdef _SHADOWMAP_DEBUG_
		RENDER_INPUT_POSTPROCESS depth_lookup;
		assert(!shadow_depthtexturelist.empty());
		depth_lookup.SetShader(&shadermap["simple"]);
		depth_lookup.SetSourceTexture(shadow_depthtexturelist.back().RenderToFBO());
		Render(&depth_lookup, final, error_output);
		#endif

		SendDrawlistToRenderScene(renderscene,&only2d);
		Render(&renderscene, final, error_output);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, 45.0, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,&camtransfilter);
		Render(&renderscene, final, error_output);
	}
	else //non-shader path
	{
		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes
		SendDrawlistToRenderScene(renderscene,&skyboxes_noblend);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		SendDrawlistToRenderScene(renderscene,&skyboxes_blend);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,&no2d_noblend);
		renderscene.Render(glstate);

		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,&no2d_blend);
		renderscene.Render(glstate);

		SendDrawlistToRenderScene(renderscene,&only2d);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, 45.0, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,&camtransfilter);
		renderscene.Render(glstate);
	}
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
	output.Begin(error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output begin", error_output);

	input->Render(glstate);

	OPENGL_UTILITY::CheckForOpenGLErrors("render finish", error_output);

	output.End(error_output);

	OPENGL_UTILITY::CheckForOpenGLErrors("render output end", error_output);
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

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::Render(GLSTATEMANAGER & glstate)
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	if (orthomode)
	{
		glOrtho(orthomin[0], orthomax[0], orthomin[1], orthomax[1], orthomin[2], orthomax[2]);
		//std::cout << "ortho near/far: " << orthomin[2] << "/" << orthomax[2] << std::endl;
	}
	else
	{
		gluPerspective( camfov, w/(float)h, 0.1f, lod_far );
	}
	glMatrixMode( GL_MODELVIEW );
	float temp_matrix[16];
	(cam_rotation).GetMatrix4(temp_matrix);
	glLoadMatrixf(temp_matrix);
	glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);

	//send information to the shaders
	if (shaders)
	{
		//camera transform goes in texture3
		glActiveTextureARB(GL_TEXTURE3_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		(cam_rotation).GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);

		//cubemap transform goes in texture2
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5,1,0,0);
		camlook.Rotate(-3.141593*0.5,0,0,1);
		QUATERNION <float> cuberotation;
		cuberotation = (-camlook) * (-cam_rotation); //experimentally derived
		(cuberotation).GetMatrix4(temp_matrix);
		//(cam_rotation).GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		//glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);
		//glLoadIdentity();

		/*glActiveTextureARB(GL_TEXTURE3_ARB);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		(cam_rotation).GetMatrix4(temp_matrix);
		glLoadMatrixf(temp_matrix);
		glTranslatef(-cam_position[0],-cam_position[1],-cam_position[2]);*/

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glMatrixMode(GL_MODELVIEW);

		//send light position to the shaders
		MATHVECTOR <float, 3> lightvec = lightposition;
		(cam_rotation).RotateVector(lightvec);
		//(cuberotation).RotateVector(lightvec);
		shadermap[SHADER_FULL]->UploadActiveShaderParameter3f("lightposition", lightvec[0], lightvec[1], lightvec[2]);
		shadermap[SHADER_FULL]->UploadActiveShaderParameter1f("contrast", contrast);
		shadermap[SHADER_FULLBLEND]->UploadActiveShaderParameter3f("lightposition", lightvec[0], lightvec[1], lightvec[2]);
		shadermap[SHADER_FULLBLEND]->UploadActiveShaderParameter1f("contrast", contrast);
		shadermap[SHADER_SKYBOX]->UploadActiveShaderParameter1f("contrast", contrast);
		/*float lightarray[3];
		for (int i = 0; i < 3; i++)
		lightarray[i] = lightvec[i];
		glLightfv(GL_LIGHT0, GL_POSITION, lightarray);*/

		glActiveTextureARB(GL_TEXTURE2_ARB);
		if (reflection && reflection->Loaded())
		{
			reflection->Activate();
		}
		else
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB,0);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		
		glActiveTextureARB(GL_TEXTURE3_ARB);
		if (ambient && ambient->Loaded())
		{
			ambient->Activate();
		}
		else
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB,0);
			//assert(0);
		}
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}

	if (clearcolor && cleardepth)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if (clearcolor)
		glClear(GL_COLOR_BUFFER_BIT);
	else if (cleardepth)
		glClear(GL_DEPTH_BUFFER_BIT);
	
	if (depth_mode_equal)
		glDepthFunc( GL_EQUAL );
	else
		glDepthFunc( GL_LEQUAL );

	DrawList(glstate);
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::DrawList(GLSTATEMANAGER & glstate)
{
	//std::cout << drawlist.size() << endl;

	//unsigned int cullcount = 0;
	
	ExtractFrustum();
	
	unsigned int already_culled = CombineDrawlists();

	last_transform_valid = false;
	activeshader = SHADER_NONE;

	unsigned int drawcount = 0;
	unsigned int loopcount = 0;
	
	for (vector <SCENEDRAW*>::iterator ptr = combined_drawlist_cache.begin(); ptr != combined_drawlist_cache.end(); ++ptr, ++loopcount)
	{
		SCENEDRAW * i = *ptr;
		if (i->IsDraw())
		{
			if (loopcount < already_culled || !FrustumCull(*i))
			{
				drawcount++;
				
				SelectFlags(*i, glstate);

				if (shaders) SelectAppropriateShader(*i);

				SelectTexturing(*i, glstate);

				bool need_pop = SelectTransformStart(*i, glstate);

				//assert(i->GetDraw()->GetVertArray() || i->GetDraw()->IsDrawList() || !i->GetDraw()->GetLine().empty());

				if (i->GetDraw()->IsDrawList())
				{
					//cout << "beep" << endl;
					//assert(i->GetDraw()->GetModel()->HaveListID());
					//glCallList(i->GetDraw()->GetModel()->GetListID());
					for_each (i->GetDraw()->GetDrawLists().begin(), i->GetDraw()->GetDrawLists().end(), glCallList);
				}
				else if (i->GetDraw()->GetVertArray())
				{
					glEnableClientState(GL_VERTEX_ARRAY);

					const float * verts;
					unsigned int counter;	// now responsible for vertices
					i->GetDraw()->GetVertArray()->GetVertices(verts, counter);
					glVertexPointer(3, GL_FLOAT, 0, verts);

					const float * norms;
					counter = 0;	// now responsible for normals
					i->GetDraw()->GetVertArray()->GetNormals(norms, counter);
					glNormalPointer(GL_FLOAT, 0, norms);
					if (counter > 0)
						glEnableClientState(GL_NORMAL_ARRAY);

					//const float * tc[i->GetDraw()->varray.GetTexCoordSets()];
					//int tccount[i->GetDraw()->varray.GetTexCoordSets()];
					const float * tc[1];
					unsigned int tccount[1];
					if (i->GetDraw()->GetVertArray()->GetTexCoordSets() > 0)
					{
						i->GetDraw()->GetVertArray()->GetTexCoords(0, tc[0], tccount[0]);
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glTexCoordPointer(2, GL_FLOAT, 0, tc[0]);
					}

					const int * faces;
					counter = 0;	// now responsible for faces
					i->GetDraw()->GetVertArray()->GetFaces(faces, counter);

					glDrawElements(GL_TRIANGLES, counter, GL_UNSIGNED_INT, faces);

					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
				else if (!i->GetDraw()->GetLine().empty())
				{
					glstate.Enable(GL_LINE_SMOOTH);
					glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
					glLineWidth(i->GetDraw()->GetLinesize());
					glBegin(GL_LINE_STRIP);
					const std::vector< MATHVECTOR < float , 3 > > & line = i->GetDraw()->GetLine();
					for (std::vector< MATHVECTOR < float , 3 > >::const_iterator i = line.begin(); i != line.end(); ++i)
						glVertex3f((*i)[0],(*i)[1],(*i)[2]);
					glEnd();
				}

				SelectTransformEnd(*i, need_pop);
			}
		}
		/*else if (i->IsTransform())
		{
			//cout << "is transform" << endl;
			float mat[16];
			glPushMatrix();
			MATHVECTOR <float, 3> translation;
			translation = i->GetTransform()->GetTranslation();
			glTranslatef(translation[0],translation[1],translation[2]);
			i->GetTransform()->GetRotation().GetMatrix4(mat);
			glMultMatrixf(mat);
			last_transform_valid = false;
		}
		else if (i->IsEmpty())
		{
			//cout << "is transform pop" << endl;
			glPopMatrix();
		}*/
	}
	
	//std::cout << "drew " << drawcount << " of " << drawlist_static->size() + drawlist_dynamic->size() << " ("  << combined_drawlist_cache.size() << "/" << already_culled << ")" << std::endl;

	if (last_transform_valid)
		glPopMatrix();
}

float accum_square(float till_now, float next){
	return till_now + next*next;
}

///returns true if the object was culled and should not be drawn
bool GRAPHICS_SDLGL::RENDER_INPUT_SCENE::FrustumCull(SCENEDRAW & tocull) const
{
	//return false;

	const SCENEDRAW * i (& tocull);
	const DRAWABLE * d (i->GetDraw());
	//if (d->GetRadius() != 0.0 && d->parent != NULL && !d->skybox)
	if (d->GetRadius() != 0.0 && !d->GetSkybox() && d->GetCameraTransformEnable())
	{
		//do frustum culling
		MATHVECTOR <float, 3> objpos(d->GetObjectCenter());
		if (i->IsCollapsed())
			i->GetMatrix4()->TransformVectorOut(objpos[0],objpos[1],objpos[2]);
		else { //if (d->GetParent() != NULL)
			assert(d->GetParent() != NULL);
			objpos = d->GetParent()->TransformIntoWorldSpace(objpos);
		}
		float dr[3] = {objpos[0]-cam_position[0], objpos[1]-cam_position[1], objpos[2]-cam_position[2]};
		float rc=std::accumulate(dr, dr+3, 0, accum_square);
		float bound = d->GetRadius();
		float temp_lod_far = lod_far + bound;
		if (rc < bound*bound)
			return false;
		else if (rc > temp_lod_far*temp_lod_far)
			return true;
		else
		{
			float rd;
			int num_of_trues = 0;
			#pragma omp parallel for reduction(+:num_of_trues) private(rd)
			for (int i=0; i<6; i++)
			{
				rd=frustum[i][0]*objpos[0]+
						frustum[i][1]*objpos[1]+
						frustum[i][2]*objpos[2]+
						frustum[i][3];
				if (rd <= -bound)
				{
					++num_of_trues;
				}
			}
			if (num_of_trues>0) return true;
		}
	}

	return false;
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SelectAppropriateShader(SCENEDRAW & forme)
{
	if (forme.GetDraw()->Get2D())
	{
		if (forme.GetDraw()->GetDistanceField())
			SetActiveShader(SHADER_DISTANCEFIELD);
		else
			SetActiveShader(SHADER_SIMPLE);
	}
	else
	{
		if (forme.GetDraw()->GetSkybox() || !forme.GetDraw()->GetLit())
			SetActiveShader(SHADER_SKYBOX);
		else if (forme.GetDraw()->GetSmoke())
			SetActiveShader(SHADER_SIMPLE);
		else
		{
			bool blend = (forme.GetDraw()->GetDecal() || forme.GetDraw()->GetPartialTransparency());
			if (blend)
				SetActiveShader(SHADER_FULLBLEND);
			else
				SetActiveShader(SHADER_FULL);
		}
	}
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SelectFlags(SCENEDRAW & forme, GLSTATEMANAGER & glstate)
{
	SCENEDRAW * i(&forme);
	if (i->GetDraw()->GetDecal() || i->GetDraw()->GetPartialTransparency())
	{
		glstate.Enable(GL_POLYGON_OFFSET_FILL);
	}
	else
	{
		glstate.Disable(GL_POLYGON_OFFSET_FILL);
	}

	if (i->GetDraw()->GetCull())
	{
		glstate.Enable(GL_CULL_FACE);
		if (i->GetDraw()->GetCull())
		{
			if (i->GetDraw()->GetCullFront())
				glstate.SetCullFace(GL_FRONT);
			else
				glstate.SetCullFace(GL_BACK);
		}
	}
	else
		glstate.Disable(GL_CULL_FACE);

	bool blend = (i->GetDraw()->GetDecal() || i->GetDraw()->Get2D() ||
			i->GetDraw()->GetPartialTransparency() || i->GetDraw()->GetDistanceField());

	if (blend && !i->GetDraw()->GetForceAlphaTest())
	{
		if (fsaa > 1)
		{
			glstate.Disable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
		}

		//if (!shaders && i->GetDraw()->GetDistanceField())
		//{
		//	glstate.Enable(GL_ALPHA_TEST);
		//	glAlphaFunc(GL_GREATER, 0.5f);
		//}
		//else
		glstate.Disable(GL_ALPHA_TEST);
		glstate.Enable(GL_BLEND);
		
		//glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		/*if (shaders)
		{
			glstate.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		else*/
		{
			glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if (i->GetDraw()->GetSmoke())
				glstate.SetBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}
	}
	else
	{
		if (fsaa > 1)
		{
			glstate.Enable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
		}
			/*glstate.Enable(GL_BLEND);
			glstate.Disable(GL_ALPHA_TEST);

			glstate.Disable(GL_BLEND);
			glstate.Enable(GL_ALPHA_TEST);*/
		/*}
		else
		{*/
			//glstate.Enable(GL_BLEND);
			glstate.Disable(GL_BLEND);
			if (i->GetDraw()->GetDistanceField())
				glstate.SetAlphaFunc(GL_GREATER, 0.5f);
			else
				glstate.SetAlphaFunc(GL_GREATER, 0.25f);
			glstate.Enable(GL_ALPHA_TEST);
		//}
	}

	if (i->GetDraw()->GetSmoke())// || i->GetDraw()->GetSkybox()) // commented out because the depth buffer will now be cleared after rendering skyboxes
	{
		glstate.SetDepthMask(false);
	}
	else
	{
		glstate.SetDepthMask(true);
	}

	//if (i->GetDraw()->GetSmoke() || i->GetDraw()->Get2D() || i->GetDraw()->GetSkybox())
	if (i->GetDraw()->GetSmoke() || i->GetDraw()->Get2D())
		glstate.Disable(GL_DEPTH_TEST);
	else
		glstate.Enable(GL_DEPTH_TEST);

	float r,g,b,a;
	i->GetDraw()->GetColor(r,g,b,a);
	glstate.SetColor(r,g,b,a);
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SelectTexturing(SCENEDRAW & forme, GLSTATEMANAGER & glstate)
{
	SCENEDRAW * i(&forme);

	bool enabletex = true;

	const TEXTURE_GL * diffusetexture = i->GetDraw()->GetDiffuseMap();

	if (!diffusetexture)
		enabletex = false;
	else
		if (!diffusetexture->Loaded())
			enabletex = false;

	if (!enabletex)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glstate.Disable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		return;
	}
	
	//if (!enabletex) std::cout << "Unloaded texture error: " << i->GetDraw()->GetDiffuseMap()->GetTextureInfo().GetName() << std::endl;
	//assert(enabletex); //don't draw without a texture

	if (enabletex)
	{
		//if (current_diffusemap != &(i->GetDraw()->diffuse_map.GetPtr()->GetTexture()))
		{
			glActiveTextureARB(GL_TEXTURE0_ARB);

			glstate.Enable(GL_TEXTURE_2D);

			i->GetDraw()->GetDiffuseMap()->Activate();

			//cout << "boop" << endl;

			//current_diffusemap = &(i->GetDraw()->GetDiffuseMap()->GetTexture());

			if (shaders)
			{
				//send the width and height to the shader in pixels.  we don't check for success of the upload, because some shaders may not want the parameter.
				//shadermap[activeshader]->UploadActiveShaderParameter1f("diffuse_texture_width", i->GetDraw()->GetDiffuseMap()->GetW());
				//shadermap[activeshader]->UploadActiveShaderParameter1f("diffuse_texture_height", i->GetDraw()->GetDiffuseMap()->GetH());

				glActiveTextureARB(GL_TEXTURE1_ARB);
				if (i->GetDraw()->GetMiscMap1())
					i->GetDraw()->GetMiscMap1()->Activate();
				else
					glBindTexture(GL_TEXTURE_2D,0);

				glActiveTextureARB(GL_TEXTURE8_ARB);
				if (i->GetDraw()->GetAdditiveMap1() && i->GetDraw()->GetSelfIllumination())
					i->GetDraw()->GetAdditiveMap1()->Activate();
				else
				{
					glDisable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D,0);
				}

				glActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
	}
	else
	{
		glstate.Disable(GL_TEXTURE_2D);
		//current_diffusemap = NULL;
	}
}

///returns true if the matrix was pushed
bool GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SelectTransformStart(SCENEDRAW & forme, GLSTATEMANAGER & glstate)
{
	SCENEDRAW * i(&forme);
	if (i->GetDraw()->Get2D())
	{
		if (last_transform_valid)
			glPopMatrix();
		last_transform_valid = false;

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		//glOrtho( 0, 1, 0, 1, -1, 1 );
		glOrtho( 0, 1, 1, 0, -1, 1 );
		//glOrtho( -5, 5, -5, 5, -100, 100 );
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		if (i->IsCollapsed())
		{
			glMultMatrixf(i->GetMatrix4()->GetArray());
		}
		return true;
	}
	else
	{
		glstate.Enable(GL_DEPTH_TEST);

		assert (i->IsCollapsed());
		{
			if (!i->GetDraw()->GetCameraTransformEnable()) //do our own transform only and ignore the camera position / orientation
			{
				if (last_transform_valid)
					glPopMatrix();
				last_transform_valid = false;

				glActiveTextureARB(GL_TEXTURE1_ARB);
				glMatrixMode(GL_TEXTURE);
				glPushMatrix();
				glLoadIdentity();
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glMatrixMode(GL_MODELVIEW);

				glPushMatrix();
				glLoadMatrixf(i->GetMatrix4()->GetArray());
				return true;
			}
			else if (i->GetDraw()->GetSkybox())
			{
				if (last_transform_valid)
					glPopMatrix();
				last_transform_valid = false;

				glPushMatrix();
				float temp_matrix[16];
				cam_rotation.GetMatrix4(temp_matrix);
				glLoadMatrixf(temp_matrix);
				if (i->GetDraw()->GetVerticalTrack())
				{
					MATHVECTOR< float, 3 > objpos(i->GetDraw()->GetObjectCenter());
					//std::cout << "Vertical offset: " << objpos;
					objpos = i->GetDraw()->GetParent()->TransformIntoWorldSpace(objpos);
					//std::cout << " || " << objpos << endl;
					//glTranslatef(-objpos.x,-objpos.y,-objpos.z);
					//glTranslatef(0,game.cam.position.y,0);
					glTranslatef(0.0,0.0,-objpos[2]);
				}
				glMultMatrixf(i->GetMatrix4()->GetArray());
				return true;
			}
			else
			{
				if (!last_transform_valid || !last_transform.Equals(*i->GetMatrix4()))
				{
					if (last_transform_valid)
						glPopMatrix();

					glPushMatrix();
					glMultMatrixf(i->GetMatrix4()->GetArray());
					last_transform = *i->GetMatrix4();
					last_transform_valid = true;

				}
				return false;
			}
		}
	}
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SelectTransformEnd(SCENEDRAW & forme, bool need_pop)
{
	SCENEDRAW * i(&forme);
	if (i->GetDraw()->Get2D() && need_pop)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}
	else if (i->IsCollapsed())
	{
		if (!forme.GetDraw()->GetCameraTransformEnable())
		{
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glMatrixMode(GL_TEXTURE);
			glPopMatrix();
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glMatrixMode(GL_MODELVIEW);
		}

		if (need_pop)
		{
			glPopMatrix();
			//cout << "popping" << endl;
		}
		//else cout << "not popping" << endl;
	}
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::SetCameraInfo(const MATHVECTOR <float, 3> & newpos,
		const QUATERNION <float> & newrot, float newfov, float newlodfar, float neww, float newh)
{
	cam_position = newpos;
	cam_rotation = newrot;
	camfov = newfov;
	lod_far = newlodfar;
	w = neww;
	h = newh;
}

void GRAPHICS_SDLGL::RENDER_INPUT_SCENE::ExtractFrustum()
{
	float   proj[16];
	float   modl[16];
	float   clip[16];
	float   t;

	/* Get the current PROJECTION matrix from OpenGL */
	glGetFloatv( GL_PROJECTION_MATRIX, proj );

	/* Get the current MODELVIEW matrix from OpenGL */
	glGetFloatv( GL_MODELVIEW_MATRIX, modl );

	/* Combine the two matrices (multiply projection by modelview) */
	clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
	clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
	clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
	clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

	clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
	clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
	clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
	clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

	clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
	clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
	clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

	clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

	/* Extract the numbers for the RIGHT plane */
	frustum[0][0] = clip[ 3] - clip[ 0];
	frustum[0][1] = clip[ 7] - clip[ 4];
	frustum[0][2] = clip[11] - clip[ 8];
	frustum[0][3] = clip[15] - clip[12];

	/* Normalize the result */
	t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
	frustum[0][0] /= t;
	frustum[0][1] /= t;
	frustum[0][2] /= t;
	frustum[0][3] /= t;

	/* Extract the numbers for the LEFT plane */
	frustum[1][0] = clip[ 3] + clip[ 0];
	frustum[1][1] = clip[ 7] + clip[ 4];
	frustum[1][2] = clip[11] + clip[ 8];
	frustum[1][3] = clip[15] + clip[12];

	/* Normalize the result */
	t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
	frustum[1][0] /= t;
	frustum[1][1] /= t;
	frustum[1][2] /= t;
	frustum[1][3] /= t;

	/* Extract the BOTTOM plane */
	frustum[2][0] = clip[ 3] + clip[ 1];
	frustum[2][1] = clip[ 7] + clip[ 5];
	frustum[2][2] = clip[11] + clip[ 9];
	frustum[2][3] = clip[15] + clip[13];

	/* Normalize the result */
	t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
	frustum[2][0] /= t;
	frustum[2][1] /= t;
	frustum[2][2] /= t;
	frustum[2][3] /= t;

	/* Extract the TOP plane */
	frustum[3][0] = clip[ 3] - clip[ 1];
	frustum[3][1] = clip[ 7] - clip[ 5];
	frustum[3][2] = clip[11] - clip[ 9];
	frustum[3][3] = clip[15] - clip[13];

	/* Normalize the result */
	t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
	frustum[3][0] /= t;
	frustum[3][1] /= t;
	frustum[3][2] /= t;
	frustum[3][3] /= t;

	/* Extract the FAR plane */
	frustum[4][0] = clip[ 3] - clip[ 2];
	frustum[4][1] = clip[ 7] - clip[ 6];
	frustum[4][2] = clip[11] - clip[10];
	frustum[4][3] = clip[15] - clip[14];

	/* Normalize the result */
	t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
	frustum[4][0] /= t;
	frustum[4][1] /= t;
	frustum[4][2] /= t;
	frustum[4][3] /= t;

	/* Extract the NEAR plane */
	frustum[5][0] = clip[ 3] + clip[ 2];
	frustum[5][1] = clip[ 7] + clip[ 6];
	frustum[5][2] = clip[11] + clip[10];
	frustum[5][3] = clip[15] + clip[14];

	/* Normalize the result */
	t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
	frustum[5][0] /= t;
	frustum[5][1] /= t;
	frustum[5][2] /= t;
	frustum[5][3] /= t;
}

GRAPHICS_SDLGL::RENDER_INPUT_SCENE::RENDER_INPUT_SCENE() : last_transform_valid(false), shaders(false), clearcolor(false), cleardepth(false), orthomode(false), contrast(1.0), use_static_partitioning(false), depth_mode_equal(false)
{
	shadermap.resize(SHADER_NONE, NULL);
	MATHVECTOR <float, 3> front(1,0,0);
	lightposition = front;
	QUATERNION <float> ldir;
				//ldir.Rotate(3.141593*0.4,0,0,1);
				//ldir.Rotate(3.141593*0.5*0.7,1,0,0);
	ldir.Rotate(3.141593*0.5,1,0,0);
	ldir.RotateVector(lightposition);
}

bool TextureSort(const SCENEDRAW & draw1, const SCENEDRAW & draw2)
{
	return (draw1.GetDraw()->GetDiffuseMap()->GetTextureInfo().GetName() < draw2.GetDraw()->GetDiffuseMap()->GetTextureInfo().GetName());
}

void GRAPHICS_SDLGL::OptimizeStaticDrawlistmap()
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
}

SCENEDRAW * PointerTo(const SCENEDRAW & sd)
{
	return const_cast<SCENEDRAW *> (&sd);
}

unsigned int GRAPHICS_SDLGL::RENDER_INPUT_SCENE::CombineDrawlists()
{
	combined_drawlist_cache.resize(0);
	combined_drawlist_cache.reserve(drawlist_static->size()+drawlist_dynamic->size());
	
	if (use_static_partitioning)
	{
		AABB<float>::FRUSTUM aabbfrustum(frustum);
		//aabbfrustum.DebugPrint(std::cout);
		static_partitioning->Query(aabbfrustum, combined_drawlist_cache);
		calgo::transform(*drawlist_dynamic, std::back_inserter(combined_drawlist_cache), &PointerTo);
		return combined_drawlist_cache.size();
	} else {
		calgo::transform(*drawlist_static, std::back_inserter(combined_drawlist_cache), &PointerTo);
		calgo::transform(*drawlist_dynamic, std::back_inserter(combined_drawlist_cache), &PointerTo);
		return 0;
	}
}
