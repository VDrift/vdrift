#include "graphics_gl3v.h"
#include "joeserialize.h"

#include <sstream>
#include <vector>
#include <map>
#include <tr1/unordered_map>

#define enableContributionCull true

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
	bool success = ReloadShaders(shaderpath, info_output, error_output);
	initialized = success;
	return success;
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

void GRAPHICS_GL3V::SetupScene(float fov, float new_view_distance, const MATHVECTOR <float, 3> cam_position, const QUATERNION <float> & cam_rotation,
				const MATHVECTOR <float, 3> & dynamic_reflection_sample_pos)
{
	lastCameraPosition = cam_position;
	
	MATRIX4 <float> viewMatrix;
	(cam_rotation).GetMatrix4(viewMatrix);
	MATHVECTOR <float, 3> rotated_cam_position = cam_position;
	cam_rotation.RotateVector(rotated_cam_position);
	viewMatrix.Translate(-rotated_cam_position[0],-rotated_cam_position[1],-rotated_cam_position[2]);
	
	MATRIX4 <float> projectionMatrix;
	projectionMatrix.Perspective(fov, w/(float)h, 0.1f, new_view_distance);
	
	renderer.setPassUniform(stringMap.addStringId("Normal"), RenderUniformEntry(stringMap.addStringId("viewMatrix"), viewMatrix.GetArray(),16));
	renderer.setPassUniform(stringMap.addStringId("Normal"), RenderUniformEntry(stringMap.addStringId("projectionMatrix"), projectionMatrix.GetArray(),16));
	
	//TODO: more camera setup
}

// returns true for cull, false for don't-cull
bool contributionCull(const DRAWABLE * d, const MATHVECTOR <float, 3> & cam)
{
	const MATHVECTOR <float, 3> & obj = d->GetObjectCenter();
	float radius = d->GetRadius();
	float dist2 = (obj - cam).MagnitudeSquared();
	const float fov = 90; // rough field-of-view estimation
	float numerator = 2*radius*fov;
	const float pixelThreshold = 1;
	//float pixels = numerator*numerator/dist2; // perspective divide (we square the numerator because we're using squared distance)
	//if (pixels < pixelThreshold)
	if (numerator*numerator < dist2*pixelThreshold)
		return true;
	else
		return false;
}

struct DrawGroupAssembler
{
	DrawGroupAssembler(StringIdMap & stringIdMap,
					   GLWrapper & glwrapper,
					   std::map <StringId, std::vector <RenderModelExternal*> > & output) 
		: stringMap(stringIdMap), gl(glwrapper), drawGroups(output), frustum(NULL), contributionCullCount(0) {}
		
	StringIdMap & stringMap;
	GLWrapper & gl;
	std::map <StringId, std::vector <RenderModelExternal*> > & drawGroups;
	FRUSTUM * frustum;
	MATHVECTOR <float, 3> camPos;
	int contributionCullCount;
	
	void operator()(const std::string & name, const std::vector <DRAWABLE*> & drawables)
	{
		std::vector <RenderModelExternal*> & out = drawGroups[stringMap.addStringId(name)];
		if (frustum && enableContributionCull)
		{
			for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
			{
				if (!contributionCull(*i, camPos))
					out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
				else
					contributionCullCount++;
			}
		}
		else
		{
			for (std::vector <DRAWABLE*>::const_iterator i = drawables.begin(); i != drawables.end(); i++)
			{
				out.push_back(&(*i)->generateRenderModelData(gl, stringMap));
			}
		}
	}
	
	void operator()(const std::string & name, const AABB_SPACE_PARTITIONING_NODE_ADAPTER <DRAWABLE> & adapter)
	{
		static std::vector <DRAWABLE*> queryResults;
		queryResults.clear();
		if (frustum)
			adapter.Query(*frustum, queryResults);
		else
			adapter.Query(AABB<float>::INTERSECT_ALWAYS(), queryResults);
		
		operator()(name, queryResults);
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
	
	DrawGroupAssembler assembler(stringMap, gl, drawGroups);
	
	// setup the dynamic draw list for rendering
	dynamic_drawlist.ForEachWithName(assembler);
	
	// setup the static draw list for rendering
	RenderUniform proj, view;
	bool doCull = true;
	StringId passName = stringMap.addStringId("Normal");
	std::string drawGroupName = "normal_noblend";
	doCull = !(!doCull || !renderer.getPassUniform(passName, stringMap.addStringId("viewMatrix"), view));
	doCull = !(!doCull || !renderer.getPassUniform(passName, stringMap.addStringId("projectionMatrix"), proj));
	FRUSTUM frustum;
	if (doCull)
	{
		frustum.Extract(&proj.data[0], &view.data[0]);
		assembler.frustum = &frustum;
		assembler.camPos = lastCameraPosition;
	}
	
	assembler(drawGroupName, *static_drawlist.GetDrawlist().GetByName(drawGroupName));
	
	/*for (std::map <StringId, std::vector <RenderModelExternal*> >::iterator i = drawGroups.begin(); i != drawGroups.end(); i++)
	{
		std::cout << stringMap.getString(i->first) << ": " << i->second.size() << std::endl;
	}
	std::cout << "----------" << std::endl;
	if (enableContributionCull) std::cout << "Contribution cull count: " << assembler.contributionCullCount << std::endl;*/
	std::cout << "normal_noblend: " << drawGroups[stringMap.addStringId("normal_noblend")].size() << "/" << static_drawlist.GetDrawlist().GetByName("normal_noblend")->size() << std::endl;
	
	// render!
	gl.logging(logNextGlFrame);
	renderer.render(w, h, stringMap, drawGroups, error_output);
	gl.logging(false);
	
	logNextGlFrame = false;
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
			if (initialized)
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
	
	if (initialized)
		logNextGlFrame = true;
	
	return true;
}

void GRAPHICS_GL3V::AddStaticNode(SCENENODE & node, bool clearcurrent)
{
	static_drawlist.Generate(node, clearcurrent);
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
