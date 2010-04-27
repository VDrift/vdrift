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
	shader_load_success = shader_load_success && LoadShader(shaderpath, "simplecube", info_output, error_output);
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
				depthFBO.Init(shadow_resolution, shadow_resolution, FBTEXTURE_GL::NORMAL, false, false, false, false, error_output);
#else
				if (shadow_quality < 4 || count > 0)
					depthFBO.Init(shadow_resolution, shadow_resolution, FBTEXTURE_GL::NORMAL, true, false, false, false, error_output); //depth texture
				else
					depthFBO.Init(shadow_resolution, shadow_resolution, FBTEXTURE_GL::NORMAL, false, true, false, false, error_output); //RGBA texture w/ nearest neighbor filtering
#endif
				count++;
			}
		}

		if (lighting == 1)
		{
			int map_size = 1024;
			FBTEXTURE_GL & depthFBO = edgecontrastenhancement_depths.RenderToFBO();
			depthFBO.Init(map_size, map_size, FBTEXTURE_GL::NORMAL, true, false, false, false, error_output);
		}

		if (bloom) //generate a screen-sized rectangular FBO
		{
			FBTEXTURE_GL & sceneFBO = full_scene_buffer.RenderToFBO();
			sceneFBO.Init(w, h, FBTEXTURE_GL::RECTANGLE, false, false, true, false, error_output);
		}

		if (bloom) //generate a buffer to store the bloom result plus a buffer for separable blur intermediate step
		{
			int bloom_size = 512;
			FBTEXTURE_GL & bloomFBO = bloom_buffer.RenderToFBO();
			bloomFBO.Init(bloom_size, bloom_size, FBTEXTURE_GL::NORMAL, false, false, false, false, error_output);

			FBTEXTURE_GL & blurFBO = blur_buffer.RenderToFBO();
			blurFBO.Init(bloom_size, bloom_size, FBTEXTURE_GL::NORMAL, false, false, false, false, error_output);
		}
		
		if (reflection_status == REFLECTION_DYNAMIC)
		{
			int reflection_size = 256;
			FBTEXTURE_GL & reflection_cubemap = dynamic_reflection.RenderToFBO();
			reflection_cubemap.Init(reflection_size, reflection_size, FBTEXTURE_GL::CUBEMAP, false, false, false, false, error_output);
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

void GRAPHICS_SDLGL::SendDrawlistToRenderScene(RENDER_INPUT_SCENE & renderscene, std::vector <DRAWABLE*> & drawlist)
{
	renderscene.SetDrawList(drawlist);
}

bool SortDraworder(DRAWABLE * d1, DRAWABLE * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

void GRAPHICS_SDLGL::DrawScene(std::ostream & error_output)
{
	renderscene.SetFlags(using_shaders);
	renderscene.SetFSAA(fsaa);
	renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);

	if (reflection_status == REFLECTION_STATIC)
		renderscene.SetReflection(&static_reflection);
	renderscene.SetAmbient(static_ambient);
	renderscene.SetContrast(contrast);

	//pre-process the drawlists
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);

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
				shadowbox = shadowbox * (shadow_radius*sqrt(2.0));
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
				SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_noblend);
				Render(&renderscene, *i, error_output);
				SendDrawlistToRenderScene(renderscene,dynamic_drawlist.car_noblend);
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
			SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_noblend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			renderscene.SetClear(false, false);
			SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_blend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);
			SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_noblend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			SendDrawlistToRenderScene(renderscene,dynamic_drawlist.car_noblend);
			Render(&renderscene, edgecontrastenhancement_depths, error_output);
			//SendDrawlistToRenderScene(renderscene,&no2d_blend);
			//Render(&renderscene, edgecontrastenhancement_depths, error_output);

			//load the texture
			glActiveTexture(GL_TEXTURE7);
			glEnable(GL_TEXTURE_2D);
			edgecontrastenhancement_depths.RenderToFBO().Activate();
			glActiveTexture(GL_TEXTURE0);
		}
		
		//dynamic reflection cubemap pre-pass to generate reflection map
		if (reflection_status == REFLECTION_DYNAMIC)
		{
			OPENGL_UTILITY::CheckForOpenGLErrors("reflection map generation: begin", error_output);
			
			FBTEXTURE_GL & reflection_fbo = dynamic_reflection.RenderToFBO();
			
			const float pi = 3.141593;
			
			for (int i = 0; i < 6; i++)
			{
				QUATERNION <float> orient;
				
				// set up our target
				switch (i)
				{
					case 0:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::POSX);
					orient.Rotate(pi*0.5, 0,1,0);
					break;
					
					case 1:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::NEGX);
					orient.Rotate(-pi*0.5, 0,1,0);
					break;
					
					case 2:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::POSY);
					orient.Rotate(pi*0.5, 1,0,0);
					break;
					
					case 3:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::NEGY);
					orient.Rotate(-pi*0.5, 1,0,0);
					break;
					
					case 4:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::POSZ);
					// orient is already set up for us!
					break;
					
					case 5:
					reflection_fbo.SetCubeSide(FBTEXTURE_GL::NEGZ);
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
				SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_noblend);
				Render(&renderscene, dynamic_reflection, error_output);
				renderscene.SetClear(false, false);
				SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_blend);
				Render(&renderscene, dynamic_reflection, error_output);
				renderscene.SetCameraInfo(dynamic_reflection_sample_position, orient, fov, 100.0, rw, rh); //use a smaller draw distance than normal
				SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_noblend);
				Render(&renderscene, dynamic_reflection, error_output);
			}
			
			OPENGL_UTILITY::CheckForOpenGLErrors("reflection map generation: end", error_output);
			
			// set it up to be used
			renderscene.SetReflection(&reflection_fbo);
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

		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_noblend);
		Render(&renderscene, *scenebuffer, error_output);
		renderscene.SetClear(false, true);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_blend);
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
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_noblend);
		Render(&renderscene, *scenebuffer, error_output);
		
		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.car_noblend);
		Render(&renderscene, *scenebuffer, error_output);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_blend);
		Render(&renderscene, *scenebuffer, error_output);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.particle);
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
		
		// debugging for dynamic reflections
		#ifdef _DYNAMIC_REFLECT_DEBUG_
		if (reflection_status == REFLECTION_DYNAMIC)
		{
			FBTEXTURE_GL & reflection_fbo = dynamic_reflection.RenderToFBO();
			RENDER_INPUT_POSTPROCESS cubelookup;
			cubelookup.SetShader(&shadermap["simplecube"]);
			cubelookup.SetSourceTexture(reflection_fbo);
			Render(&cubelookup, final, error_output);
		}
		#endif

		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.twodim);
		Render(&renderscene, final, error_output);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.text);
		Render(&renderscene, final, error_output);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, 45.0, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.nocamtrans_noblend);
		Render(&renderscene, final, error_output);
		
		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.nocamtrans_blend);
		Render(&renderscene, final, error_output);
	}
	else //non-shader path
	{
		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, camfov, 10000.0, w, h); //use very high draw distance for skyboxes
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_noblend);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.skybox_blend);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, camfov, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_noblend);
		renderscene.Render(glstate);
		
		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.car_noblend);
		renderscene.Render(glstate);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.normal_blend);
		renderscene.Render(glstate);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.particle);
		renderscene.Render(glstate);

		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.twodim);
		renderscene.Render(glstate);
		
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.text);
		renderscene.Render(glstate);

		renderscene.SetClear(false, true);
		renderscene.SetCameraInfo(campos, camorient, 45.0, view_distance, w, h);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.nocamtrans_noblend);
		renderscene.Render(glstate);
		
		renderscene.SetClear(false, false);
		SendDrawlistToRenderScene(renderscene,dynamic_drawlist.nocamtrans_blend);
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

bool TextureSort(const DRAWABLE & draw1, const DRAWABLE & draw2)
{
	return (draw1.GetDiffuseMap()->GetTextureInfo().GetName() < draw2.GetDiffuseMap()->GetTextureInfo().GetName());
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
