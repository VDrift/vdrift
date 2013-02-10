/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "trackmap.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"

#ifdef __APPLE__
#include <SDL_gfx/SDL_gfxPrimitives.h>
#else
#include <SDL/SDL_gfxPrimitives.h>
#endif

#include "float.h"

using std::endl;
using std::string;
using std::ostream;
using std::list;
using std::vector;
using std::pair;

TRACKMAP::TRACKMAP() :
	scale(1.0), MAP_WIDTH(256), MAP_HEIGHT(256)
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
}

bool TRACKMAP::BuildMap(
	const std::list <ROADSTRIP> & roads,
	int w, int h,
	const std::string & trackname,
	const std::string & texturepath,
	ContentManager & content,
	std::ostream & error_output)
{
	Unload();

	int outsizex = MAP_WIDTH;
	int outsizey = MAP_HEIGHT;
	int bpp = 32;

	// SDL interprets each pixel as a 32-bit number, so our masks must depend
	// on the endianness (byte order) of the machine
	Uint32 rmask, gmask, bmask, amask;
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

	SDL_Surface * surface = SDL_CreateRGBSurface(SDL_SWSURFACE, outsizex, outsizey, bpp, rmask, gmask, bmask, amask);

	//find the map width and height
	mapsize.Set(0,0);
	map_w_min = FLT_MAX;
	map_w_max = FLT_MIN;
	map_h_min = FLT_MAX;
	map_h_max = FLT_MIN;

	for (list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (vector<ROADPATCH>::const_iterator curp = road->GetPatches().begin();
			curp != road->GetPatches().end(); curp++)
		{
			const BEZIER & b = curp->GetPatch();
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					const MATHVECTOR <float, 3> & p = b[i + j * 4];
					if (p[1] < map_w_min)
					{
						map_w_min = p[1];
					}
					if (p[1] > map_w_max)
					{
						map_w_max = p[1];
					}
					if (p[0] < map_h_min)
					{
						map_h_min = p[0];
					}
					if (p[0] > map_h_max)
					{
						map_h_max = p[0];
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
	scale = (scale_w < scale_h) ? scale_w : scale_h;

	boxRGBA(surface, 0, 0, outsizex-1, outsizey-1, 0, 0, 0, 0);

	for (list <ROADSTRIP>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (vector<ROADPATCH>::const_iterator curp = road->GetPatches().begin();
			curp != road->GetPatches().end(); curp++)
		{
			Sint16 x[4], y[4];

			const BEZIER & b = curp->GetPatch();
			const MATHVECTOR <float, 3> & back_l = b.GetBL();
			const MATHVECTOR <float, 3> & back_r = b.GetBR();
			const MATHVECTOR <float, 3> & front_l = b.GetFL();
			const MATHVECTOR <float, 3> & front_r = b.GetFR();

			x[0] = int((back_l[1] - map_w_min) * scale) + 1;
			y[0] = int((back_l[0] - map_h_min) * scale) + 1;
			x[1] = int((front_l[1] - map_w_min) * scale) + 1;
			y[1] = int((front_l[0] - map_h_min) * scale) + 1;
			x[2] = int((front_r[1] - map_w_min) * scale) + 1;
			y[2] = int((front_r[0] - map_h_min) * scale) + 1;
			x[3] = int((back_r[1] - map_w_min) * scale) + 1;
			y[3] = int((back_r[0] - map_h_min) * scale) + 1;
			filledPolygonRGBA(surface, x, y, 4, 255, 255, 255, 255);
			//aapolygonRGBA(surface, x, y, 4, 255, 255, 255, 255); // draw artifacts with SDL2
		}
	}

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

	TEXTUREINFO texinfo;
	texinfo.data = static_cast<unsigned char*>(surface->pixels);
	texinfo.width = surface->w;
	texinfo.height = surface->h;
	texinfo.bytespp = surface->format->BitsPerPixel / 8;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	std::tr1::shared_ptr<TEXTURE> track_map;
	content.load(track_map, "", trackname, texinfo);

	SDL_FreeSurface(surface);

	//std::cout << "Loading track map dots" << std::endl;
	TEXTUREINFO dotinfo;
	content.load(cardot0, texturepath, "cardot0.png", dotinfo);
	content.load(cardot1, texturepath, "cardot1.png", dotinfo);
	content.load(cardot0_focused, texturepath, "cardot0_focused.png", dotinfo);
	content.load(cardot1_focused, texturepath, "cardot1_focused.png", dotinfo);

	// calculate map position, size
	screen[0] = (float)w;
	screen[1] = (float)h;
	position[0] = 1.0 - MAP_WIDTH / screen[0];
	position[1] = 0.12;
	size[0] = MAP_WIDTH / screen[0];
	size[1] = MAP_HEIGHT / screen[1];
	dot_size[0] = cardot0->GetW() / 2.0 / screen[0];
	dot_size[1] = cardot0->GetH() / 2.0 / screen[1];

	mapverts.SetToBillboard(position[0], position[1], position[0]+size[0], position[1]+size[1]);
	mapdraw = mapnode.GetDrawlist().twodim.insert(DRAWABLE());
	DRAWABLE & mapdrawref = mapnode.GetDrawlist().twodim.get(mapdraw);
	mapdrawref.SetDiffuseMap(track_map);
	mapdrawref.SetVertArray(&mapverts);
	mapdrawref.SetCull(false, false);
	mapdrawref.SetColor(1,1,1,0.7);
	mapdrawref.SetDrawOrder(0);

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
			std::tr1::shared_ptr<TEXTURE> tex = cardot0_focused;
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
		for (list <CARDOT>::iterator i = dot; i != dotlist.end(); ++i)
			mapnode.GetDrawlist().twodim.erase(i->GetDrawableHandle());
		dotlist.erase(dot,dotlist.end());
	}

	if (mapdraw.valid())
	{
		DRAWABLE & mapdrawref = mapnode.GetDrawlist().twodim.get(mapdraw);
		mapdrawref.SetDrawEnable(mapvisible);
	}
	for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); ++i)
		i->SetVisible(mapnode, mapvisible);

	/*for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
		i->DebugPrint(std::cout);*/
}

