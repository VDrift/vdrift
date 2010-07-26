#include "trackmap.h"

#include "contentmanager.h"
#include "textureloader.h"
#include "texture.h"

#include "float.h"
#include <SDL/SDL.h>
#ifdef __APPLE__
#include <SDL_gfx/SDL_gfxPrimitives.h>
#else
#include <SDL/SDL_gfxPrimitives.h> //part of the SDL_gfx package
#endif

TRACKMAP::TRACKMAP() :
	scale(1.0),
	MAP_WIDTH(256),
	MAP_HEIGHT(256),
	surface(NULL)
{
	// ctor
}

TRACKMAP::~TRACKMAP()
{
	Unload();
}

void TRACKMAP::Unload()
{
	dotlist.clear();
	
	mapnode.Clear();
	
	mapdraw.invalidate();

	if (surface)
	{
		SDL_FreeSurface(surface);
		surface = NULL;
	}
}

void TRACKMAP::CalcPosition(int w, int h)
{
	screen[0] = (float)w;
	screen[1] = (float)h;
	position[0] = 1.0 - MAP_WIDTH / screen[0];
	position[1] = 0.12;
	size[0] = MAP_WIDTH / screen[0];
	size[1] = MAP_HEIGHT / screen[1];
	dot_size[0] = cardot0->GetW() / 2.0 / screen[0]; 
	dot_size[1] = cardot0->GetH() / 2.0 / screen[1]; 
}

bool TRACKMAP::BuildMap(
	const std::list <ROADSTRIP> & roads,
	int w,
	int h,
	const std::string & trackname,
	const std::string & texturepath,
	const std::string & texsize,
	ContentManager & content,
	std::ostream & error_output)
{
	Unload();
	
	int outsizex = MAP_WIDTH;
	int outsizey = MAP_HEIGHT;
	
	Uint32 rmask, gmask, bmask, amask;

	// SDL interprets each pixel as a 32-bit number, so our masks must depend
	// on the endianness (byte order) of the machine
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, outsizex, outsizey, 32, rmask, gmask, bmask, amask);
	
	//find the map width and height
	mapsize.Set(0,0);
	map_w_min = FLT_MAX;
	map_w_max = FLT_MIN;
	map_h_min = FLT_MAX;
	map_h_max = FLT_MIN;

	for (std::list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (std::list <ROADPATCH>::const_iterator curp = road->GetPatchList().begin();
		     curp != road->GetPatchList().end(); curp++)
		{
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					MATHVECTOR <float, 3> p = curp->GetPatch()[i+j*4];
					if (p[0] < map_w_min)
					{
						map_w_min = p[0];
					}
					if (p[0] > map_w_max)
					{
						map_w_max = p[0];
					}
					if (p[2] < map_h_min)
					{
						map_h_min = p[2];
					}
					if (p[2] > map_h_max)
					{
						map_h_max = p[2];
					}
				}
			}
		}
	}

	mapsize[0] = map_w_max - map_w_min;
	mapsize[1] = map_h_max - map_h_min;

	//determine the scaling factor
	//we will leave a 1 pixel border
	float scale_w = (outsizex-2) / mapsize[0];
	float scale_h = (outsizey-2) / mapsize[1];
	scale = (scale_w < scale_h)?scale_w:scale_h;

	boxRGBA(surface, 0, 0, outsizex-1, outsizey-1, 0, 0, 0, 0);

	for (std::list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (std::list <ROADPATCH>::const_iterator curp = road->GetPatchList().begin();
		     curp != road->GetPatchList().end(); curp++)
		{
			Sint16 x[4], y[4];

			const BEZIER & b(curp->GetPatch());
			MATHVECTOR <float, 3> back_l = b.GetBL();
			MATHVECTOR <float, 3> back_r = b.GetBR();
			MATHVECTOR <float, 3> front_l = b.GetFL();
			MATHVECTOR <float, 3> front_r = b.GetFR();

			x[0] = int((back_l[0] - map_w_min) * scale) + 1;
			y[0] = int((back_l[2] - map_h_min) * scale) + 1;
			x[1] = int((front_l[0] - map_w_min) * scale) + 1;
			y[1] = int((front_l[2] - map_h_min) * scale) + 1;
			x[2] = int((front_r[0] - map_w_min) * scale) + 1;
			y[2] = int((front_r[2] - map_h_min) * scale) + 1;
			x[3] = int((back_r[0] - map_w_min) * scale) + 1;
			y[3] = int((back_r[2] - map_h_min) * scale) + 1;
			filledPolygonRGBA(surface, x, y, 4, 255, 255, 255, 255);
			aapolygonRGBA(surface, x, y, 4, 255, 255, 255, 255);
		}
	}

	//draw the starting line
	/*BEZIER* startpatch = track->GetLapSequence(0);
	if (startpatch)
	{
		VERTEX startline_l = startpatch->points[3][0];
		VERTEX startline_r = startpatch->points[3][3];
		int x1 = int((startline_l.x - map_w_min) * scale) + 1;
		int y1 = int((startline_l.z - map_h_min) * scale) + 1;
		int x2 = int((startline_r.x - map_w_min) * scale) + 1;
		int y2 = int((startline_r.z - map_h_min) * scale) + 1;
		aalineRGBA(surface, x1, y1, x2, y2, 255, 0, 0, 255);
		filledCircleRGBA(surface, x1, y1, 2, 255, 0, 0, 255);
		filledCircleRGBA(surface, x2, y2, 2, 255, 0, 0, 255);
	}*/

	//draw a black border around the track
	Uint32* rawpixels = (Uint32*)surface->pixels;
	Uint32 rgbmask = rmask | gmask | bmask;

	for (int x = 0; x < outsizex; x++)
	{
		for (int y = 0; y < outsizey; y++)
		{
			//if this pixel is black
			if (rawpixels[outsizex * y + x] == 0)
			{
				//if the pixel above this one is non-black
				if ((y > 0) && ((rawpixels[outsizex * (y-1) + x] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel left of this one is non-black
				if ((x > 0) && ((rawpixels[outsizex * y + x - 1] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel right of this one is non-black
				if ((x < (outsizex - 1)) && ((rawpixels[outsizex * y + x + 1] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
				//if the pixel below this one is non-black
				if ((y < (outsizey - 1)) && ((rawpixels[outsizex * (y+1) + x] & rgbmask) > 0))
				{
					//set this pixel to non-transparent
					rawpixels[outsizex * y + x] |= amask;
				}
			}
		}
	}

	TextureLoader texload;
	texload.surface = surface;
	texload.repeatu = false;
	texload.repeatv = false;
	texload.setSize(texsize);

	texload.name = trackname;
	TexturePtr track_map = content.get<TEXTURE>(texload);
	if (!track_map.get()) return false;
	
	//std::cout << "Loading track map dots" << std::endl;
	texload.surface = NULL;
	
	texload.name = texturepath + "/cardot0.png";
	cardot0 = content.get<TEXTURE>(texload);
	if (!cardot0.get()) return false;
	
	texload.name = texturepath + "/cardot1.png";
	cardot1 = content.get<TEXTURE>(texload);
	if (!cardot1.get()) return false;
	
	texload.name = texturepath + "/cardot0_focused.png";
	cardot0_focused = content.get<TEXTURE>(texload);
	if (!cardot0_focused.get()) return false;
	
	texload.name = texturepath + "/cardot1_focused.png";
	cardot1_focused = content.get<TEXTURE>(texload);
	if (!cardot1_focused.get()) return false;
	
	CalcPosition(w, h);
	
	mapdraw = mapnode.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & mapdrawref = mapnode.GetDrawlist().twodim.get(mapdraw);
	mapdrawref.SetDiffuseMap(track_map);
	mapdrawref.SetVertArray(&mapverts);
	mapdrawref.SetCull(false, false);
	mapdrawref.SetColor(1,1,1,0.7);
	mapdrawref.SetDrawOrder(0);
	mapverts.SetToBillboard(position[0], position[1], position[0]+size[0], position[1]+size[1]);
	
	return true;
}

void TRACKMAP::Update(bool mapvisible, const std::list <std::pair<MATHVECTOR <float, 3>, bool> > & carpositions)
{
	//only update car positions when the map is visible, so we get a slight speedup if the map is hidden
	if (mapvisible)
	{
		std::list <std::pair<MATHVECTOR <float, 3>, bool> >::const_iterator car = carpositions.begin();
		std::list <CARDOT>::iterator dot = dotlist.begin();
		int count = 0;
		while (car != carpositions.end())
		{
			//determine which texture to use
			TexturePtr tex = cardot0_focused;
			if (!car->second)
				tex = cardot1;
			
			//find the coordinates of the dot
			MATHVECTOR <float, 2> dotpos = position;
			dotpos[0] += ((car->first[1] - map_w_min)*scale + 1) / screen[0];
			dotpos[1] += ((car->first[0] - map_h_min)*scale + 1) / screen[1];
			MATHVECTOR <float, 2> corner1 = dotpos - dot_size;
			MATHVECTOR <float, 2> corner2 = dotpos + dot_size;
			
			if (dot == dotlist.end())
			{
				//need to insert a new dot
				dotlist.push_back(CARDOT());
				dotlist.back().Init(mapnode, tex, corner1, corner2);
				dot = dotlist.end();
				
				//std::cout << count << ". inserting new dot: " << corner1 << " || " << corner2 << endl;
			}
			else
			{
				//update existing dot
				dot->Retexture(mapnode, tex);
				dot->Reposition(corner1, corner2);
				
				//std::cout << count << ". reusing existing dot: " << corner1 << " || " << corner2 << endl;
				
				dot++;
			}
			
			car++;
			count++;
		}
		for (std::list <CARDOT>::iterator i = dot; i != dotlist.end(); ++i)
			mapnode.GetDrawlist().twodim.erase(i->GetDrawableHandle());
		dotlist.erase(dot,dotlist.end());
	}
	
	if (mapdraw.valid())
	{
		DRAWABLE & mapdrawref = mapnode.GetDrawlist().twodim.get(mapdraw);
		mapdrawref.SetDrawEnable(mapvisible);
	}
	for (std::list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); ++i)
		i->SetVisible(mapnode, mapvisible);
	
	/*for (std::list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
		i->DebugPrint(std::cout);*/
}

void TRACKMAP::CARDOT::Init(
	SCENENODE & topnode, 
	TexturePtr tex, 
	const MATHVECTOR <float, 2> & corner1, 
	const MATHVECTOR <float, 2> & corner2)
{
	dotdraw = topnode.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & drawref = GetDrawable(topnode);
	drawref.SetVertArray(&dotverts);
	drawref.SetCull(false, false);
	drawref.SetColor(1,1,1,0.7);
	drawref.SetDrawOrder(0.1);
	Retexture(topnode, tex);
	Reposition(corner1, corner2);
}

void TRACKMAP::CARDOT::Retexture(SCENENODE & topnode, TexturePtr newtex)
{
	assert(newtex.get());
	GetDrawable(topnode).SetDiffuseMap(newtex);
}

void TRACKMAP::CARDOT::Reposition(const MATHVECTOR <float, 2> & corner1, const MATHVECTOR <float, 2> & corner2)
{
	dotverts.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
}

void TRACKMAP::CARDOT::SetVisible(SCENENODE & topnode, bool visible)
{
	GetDrawable(topnode).SetDrawEnable(visible);
}

void TRACKMAP::CARDOT::DebugPrint(SCENENODE & topnode, std::ostream & out) const
{
	const DRAWABLE & drawref = GetDrawable(topnode);
	out << &drawref << ": enable=" << drawref.GetDrawEnable() << ", tex=" << drawref.GetDiffuseMap() << ", verts=" << drawref.GetVertArray() << std::endl;
}
