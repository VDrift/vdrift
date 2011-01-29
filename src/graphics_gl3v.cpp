#include "graphics_gl3v.h"
#include "joeserialize.h"

#include <sstream>
#include <vector>
#include <map>
#include <tr1/unordered_map>

bool GRAPHICS_GL3V::Init(const std::string & shaderpath,
				unsigned int resx, unsigned int resy, unsigned int bpp,
				unsigned int depthbpp, bool fullscreen, bool shaders,
				unsigned int antialiasing, bool enableshadows,
				int shadow_distance, int shadow_quality,
				int reflection_type,
				const std::string & static_reflectionmap_file,
				const std::string & static_ambientmap_file,
				int anisotropy, const std::string & texturesize,
				int lighting_quality, bool newbloom, bool newnormalmaps,
				const std::string & renderconfig,
				std::ostream & info_output, std::ostream & error_output)
{
	// first, see if we support the required gl version by attempting to initialize the GL wrapper
	gl.setInfoOutput(info_output);
	gl.setErrorOutput(error_output);
	if (!gl.initialize())
	{
		error_output << "Initialization of GL3 failed; that's OK, falling back to GL 1 or 2" << std::endl;
		return false;
	}
	
	// this information is needed to initialize the renderer in ReloadShaders
	w = resx;
	h = resy;
	
	// initialize the renderer
	return ReloadShaders(shaderpath, info_output, error_output);
}

void GRAPHICS_GL3V::Deinit()
{
	renderer.clear();
}

void GRAPHICS_GL3V::BeginScene(std::ostream & error_output)
{
	
}

DRAWABLE_CONTAINER <PTRVECTOR> & GRAPHICS_GL3V::GetDynamicDrawlist()
{
	return dynamic_drawlist;
}

void GRAPHICS_GL3V::AddStaticNode(SCENENODE & node, bool clearcurrent)
{
	static_drawlist.Generate(node, clearcurrent);
}

void GRAPHICS_GL3V::SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
				const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos)
{
	//TODO: camera setup
}

struct DrawGroupAssembler
{
	DrawGroupAssembler(StringIdMap & stringIdMap,
					   GLWrapper & glwrapper,
					   std::map <StringId, std::vector <RenderModelExternal*> > & output) 
		: stringMap(stringIdMap), gl(glwrapper), drawGroups(output) {}
		
	StringIdMap & stringMap;
	GLWrapper & gl;
	std::map <StringId, std::vector <RenderModelExternal*> > & drawGroups;
	
	void operator()(const std::string & name, const std::vector <DRAWABLE*> & drawables) const
	{
		std::vector <RenderModelExternal*> & out = drawGroups[stringMap.addStringId(name)];
		for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
		{
			out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
		}
	}
};

static bool SortDraworder(DRAWABLE * d1, DRAWABLE * d2)
{
	assert(d1 && d2);
	return (d1->GetDrawOrder() < d2->GetDrawOrder());
}

void GRAPHICS_GL3V::DrawScene(std::ostream & error_output)
{
	//sort the two dimentional drawlist so we get correct ordering
	std::sort(dynamic_drawlist.twodim.begin(),dynamic_drawlist.twodim.end(),&SortDraworder);
	
	// put the draw lists into the drawGroups map
	// this is cached to avoid extra memory allocations, but we need to make sure to clear it out each time we start
	static std::map <StringId, std::vector <RenderModelExternal*> > drawGroups;
	for (std::map <StringId, std::vector <RenderModelExternal*> >::iterator i = drawGroups.begin(); i != drawGroups.end(); i++)
	{
		i->second.clear();
	}
	
	// for now, just copy these directly over
	DrawGroupAssembler assembler(stringMap, gl, drawGroups);
	dynamic_drawlist.ForEachWithName(assembler);
	
	// render!
	renderer.render(w, h, stringMap, drawGroups, error_output);
}

void GRAPHICS_GL3V::EndScene(std::ostream & error_output)
{
	
}

int GRAPHICS_GL3V::GetMaxAnisotropy() const
{
	int max_anisotropy = 1;
	if (GLEW_EXT_texture_filter_anisotropic)
		gl.GetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
	return max_anisotropy;
}

bool GRAPHICS_GL3V::AntialiasingSupported() const
{
	return true;
}

bool GRAPHICS_GL3V::GetUsingShaders() const 
{
	return true;
}

bool GRAPHICS_GL3V::ReloadShaders(const std::string & shaderpath, std::ostream & info_output, std::ostream & error_output) 
{
	// reinitialize the entire renderer
	std::vector <RealtimeExportPassInfo> passInfos;
	bool passInfosLoaded = joeserialize::LoadObjectFromFile("passList", shaderpath+"/gl3/vdrift1.rhr", passInfos, false, true, info_output, error_output);
	if (passInfosLoaded)
	{
		bool initSuccess = renderer.initialize(passInfos, stringMap, shaderpath+"/gl3", w, h, error_output);
		if (initSuccess)
		{
			renderer.printRendererStatus(VERBOSITY_MAXIMUM, stringMap, std::cout);
		}
		else
		{
			error_output << "Initialization of GL3 renderer failed; that's OK, falling back to GL 1 or 2" << std::endl;
			return false;
		}
	}
	else
	{
		error_output << "Unable to load GL3 pass information; that's OK, falling back to GL 1 or 2" << std::endl;
		return false;
	}
	
	info_output << "GL3 initialization successful" << std::endl;
	
	return true;
}

void GRAPHICS_GL3V::SetCloseShadow ( float value )
{
	
}

bool GRAPHICS_GL3V::GetShadows() const
{
	return true;
}

void GRAPHICS_GL3V::SetSunDirection ( const QUATERNION< float >& value )
{
	
}

void GRAPHICS_GL3V::SetContrast ( float value )
{
	
}
