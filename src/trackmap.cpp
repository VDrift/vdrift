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

using std::endl;
using std::string;
using std::ostream;
using std::list;
using std::vector;
using std::pair;

TrackMap::TrackMap() :
	map_width(256),
	map_height(256),
	map_scale(1.0)
{
	// ctor
}

TrackMap::~TrackMap()
{
	Unload();
}

void TrackMap::Unload()
{
	dotlist.clear();
	mapnode.Clear();
	mapdraw.invalidate();
}

bool TrackMap::BuildMap(
	const int screen_width,
	const int screen_height,
	const std::list <RoadStrip> & roads,
	const std::string & trackname,
	const std::string & texturepath,
	ContentManager & content,
	std::ostream & /*error_output*/)
{
	Unload();

	pixel_size[0] = 1.0f / screen_width;
	pixel_size[1] = 1.0f / screen_height;

	// track aabb
	track_min[0] = +1E6;
	track_min[1] = +1E6;
	track_max[0] = -1E6;
	track_max[1] = -1E6;
	for (list <RoadStrip>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (vector<RoadPatch>::const_iterator curp = road->GetPatches().begin();
			curp != road->GetPatches().end(); curp++)
		{
			const Bezier & b = curp->GetPatch();
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					const Vec3 & p = b[i + j * 4];
					if (p[1] < track_min[0])
					{
						track_min[0] = p[1];
					}
					if (p[1] > track_max[0])
					{
						track_max[0] = p[1];
					}
					if (p[0] < track_min[1])
					{
						track_min[1] = p[0];
					}
					if (p[0] > track_max[1])
					{
						track_max[1] = p[0];
					}
				}
			}
		}
	}

	// determine the scaling factor
	// we will leave a 1 pixel border
	const float track_width = track_max[0] - track_min[0];
	const float track_height = track_max[1] - track_min[1];
	const float map_scale_w = (map_width - 2) / track_width;
	const float map_scale_h = (map_height - 2) / track_height;
	map_scale = (map_scale_w < map_scale_h) ? map_scale_w : map_scale_h;

	std::vector<unsigned> pixels(map_width * map_height, 0);
	const int stride = map_width * sizeof(unsigned);
	const unsigned color = 0xffffffff;

	for (list <RoadStrip>::const_iterator road = roads.begin(); road != roads.end(); road++)
	{
		for (vector<RoadPatch>::const_iterator curp = road->GetPatches().begin();
			curp != road->GetPatches().end(); curp++)
		{
			const Bezier & b = curp->GetPatch();
			const Vec3 & bl = b.GetBL();
			const Vec3 & br = b.GetBR();
			const Vec3 & fl = b.GetFL();
			const Vec3 & fr = b.GetFR();

			float x[6], y[6];
			x[2] = (bl[1] - track_min[0]) * map_scale + 1;
			y[2] = (bl[0] - track_min[1]) * map_scale + 1;
			x[1] = (fl[1] - track_min[0]) * map_scale + 1;
			y[1] = (fl[0] - track_min[1]) * map_scale + 1;
			x[0] = (fr[1] - track_min[0]) * map_scale + 1;
			y[0] = (fr[0] - track_min[1]) * map_scale + 1;
			x[3] = x[2];
			y[3] = y[2];
			x[4] = (br[1] - track_min[0]) * map_scale + 1;
			y[4] = (br[0] - track_min[1]) * map_scale + 1;
			x[5] = x[0];
			y[5] = y[0];

			RasterizeTriangle(x, y, color, &pixels[0], stride);
			RasterizeTriangle(x + 3, y + 3, color, &pixels[0], stride);
		}
	}
/*
	// should operate on alpha only or on each channel?
	// horizontal blur 3x3
	for (int y = 1; y < map_height - 1; ++y)
	{
		unsigned * line = &pixels[0] + y * map_width;
		unsigned p = line[0];
		for (int x = 1; x < map_width - 1; ++x)
		{
			//unsigned v = (line0[x] + (line1[x] << 1) + line2[x]) >> 2;
			unsigned v = (line[x-1] >> 2) + (line[x] >> 1) + (line[x+1] >> 2);
			line[x-1] = p;
			p = v;
		}
		line[map_width - 2] = p;
	}
	// vertical blur 3x3
	std::vector<unsigned> linep(map_width, 0);
	for (int y = 1; y < map_height - 1; ++y)
	{
		unsigned * line0 = &pixels[0] + (y - 1) * map_width;
		unsigned * line1 = &pixels[0] + y * map_width;
		unsigned * line2 = &pixels[0] + (y + 1) * map_width;
		for (int x = 1; x < map_width - 1; ++x)
		{
			//unsigned v = (line0[x] + (line1[x] << 1) + line2[x]) >> 2;
			unsigned v = (line0[x] >> 2) + (line1[x] >> 1) + (line2[x] >> 2);
			line0[x] = linep[x];
			linep[x] = v;
		}
	}
*/
	// draw a black border around the track
	const unsigned rgbmask = 0x00ffffff;
	const unsigned amask = 0xff000000;
	for (int x = 0; x < map_width; x++)
	{
		for (int y = 0; y < map_height; y++)
		{
			// if this pixel is black
			if (pixels[map_width * y + x] == 0)
			{
				// if the pixel above this one is non-black
				if ((y > 0) && ((pixels[map_width * (y-1) + x] & rgbmask) > 0))
				{
					// set this pixel to non-transparent
					pixels[map_width * y + x] |= amask;
				}
				// if the pixel left of this one is non-black
				if ((x > 0) && ((pixels[map_width * y + x - 1] & rgbmask) > 0))
				{
					// set this pixel to non-transparent
					pixels[map_width * y + x] |= amask;
				}
				// if the pixel right of this one is non-black
				if ((x < (map_width - 1)) && ((pixels[map_width * y + x + 1] & rgbmask) > 0))
				{
					// set this pixel to non-transparent
					pixels[map_width * y + x] |= amask;
				}
				// if the pixel below this one is non-black
				if ((y < (map_height - 1)) && ((pixels[map_width * (y+1) + x] & rgbmask) > 0))
				{
					// set this pixel to non-transparent
					pixels[map_width * y + x] |= amask;
				}
			}
		}
	}

	TextureInfo texinfo;
	texinfo.data = (unsigned char*)&pixels[0];
	texinfo.width = map_width;
	texinfo.height = map_height;
	texinfo.bytespp = sizeof(unsigned);
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	content.load(track_map, "", trackname, texinfo);

	//std::cout << "Loading track map dots" << std::endl;
	TextureInfo dotinfo;
	content.load(cardot0, texturepath, "cardot0.png", dotinfo);
	content.load(cardot1, texturepath, "cardot1.png", dotinfo);
	content.load(cardot0_focused, texturepath, "cardot0_focused.png", dotinfo);
	content.load(cardot1_focused, texturepath, "cardot1_focused.png", dotinfo);

	// map position on screen: right side 16 pixel padding
	map_min[0] = 1.0 - (track_width * map_scale + 16)  * pixel_size[0];
	map_min[1] = 0.5 - 0.5 * track_height * map_scale * pixel_size[1];

	map_max[0] = map_min[0] + map_width * pixel_size[0];
	map_max[1] = map_min[1] + map_height * pixel_size[1];

	dot_size[0] = 0.5 * cardot0->GetW() * pixel_size[0];
	dot_size[1] = 0.5 * cardot0->GetH() * pixel_size[1];

	mapverts.SetToBillboard(map_min[0], map_min[1], map_max[0], map_max[1]);
	mapdraw = mapnode.GetDrawList().twodim.insert(Drawable());
	Drawable & mapdrawref = mapnode.GetDrawList().twodim.get(mapdraw);
	mapdrawref.SetTextures(track_map->GetId());
	mapdrawref.SetVertArray(&mapverts);
	mapdrawref.SetCull(false);
	//mapdrawref.SetColor(1, 1, 1, 0.7);
	mapdrawref.SetDrawOrder(0);

	return true;
}

void TrackMap::Update(bool mapvisible, const std::list <std::pair<Vec3, bool> > & carpositions)
{
	//only update car positions when the map is visible, so we get a slight speedup if the map is hidden
	if (mapvisible)
	{
		std::list <std::pair<Vec3, bool> >::const_iterator car = carpositions.begin();
		std::list <CarDot>::iterator dot = dotlist.begin();
		int count = 0;
		while (car != carpositions.end())
		{
			//determine which texture to use
			std::shared_ptr<Texture> tex = cardot0_focused;
			if (!car->second)
				tex = cardot1;

			//find the coordinates of the dot
			Vec2 dotpos = map_min;
			dotpos[0] += ((car->first[1] - track_min[0]) * map_scale + 1) * pixel_size[0];
			dotpos[1] += ((car->first[0] - track_min[1]) * map_scale + 1) * pixel_size[1];
			Vec2 corner1 = dotpos - dot_size;
			Vec2 corner2 = dotpos + dot_size;

			if (dot == dotlist.end())
			{
				//need to insert a new dot
				dotlist.push_back(CarDot());
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
		for (list <CarDot>::iterator i = dot; i != dotlist.end(); ++i)
			mapnode.GetDrawList().twodim.erase(i->GetDrawableHandle());
		dotlist.erase(dot,dotlist.end());
	}

	if (mapdraw.valid())
	{
		Drawable & mapdrawref = mapnode.GetDrawList().twodim.get(mapdraw);
		mapdrawref.SetDrawEnable(mapvisible);
	}
	for (list <CarDot>::iterator i = dotlist.begin(); i != dotlist.end(); ++i)
		i->SetVisible(mapnode, mapvisible);

	/*for (list <CARDOT>::iterator i = dotlist.begin(); i != dotlist.end(); i++)
		i->DebugPrint(std::cout);*/
}

template <typename T>
inline T min(const T a, const T b, const T c)
{
	return std::min(a, std::min(b, c));
}

template <typename T>
inline T max(const T a, const T b, const T c)
{
	return std::max(a, std::max(b, c));
}

void TrackMap::RasterizeTriangle(
	const float vx[3],
	const float vy[3],
	unsigned color,
	void * color_buffer,
	int stride)
{
	// Triangle rasterizer (8x8 block) by Nicolas Capens
	// see http://devmaster.net/posts/6145/advanced-rasterization

	// 28.4 fixed-point coordinates
	const int X1 = 16.0f * vx[0] + 0.5f;
	const int X2 = 16.0f * vx[1] + 0.5f;
	const int X3 = 16.0f * vx[2] + 0.5f;

	const int Y1 = 16.0f * vy[0] + 0.5f;
	const int Y2 = 16.0f * vy[1] + 0.5f;
	const int Y3 = 16.0f * vy[2] + 0.5f;

	// Deltas
	const int DX12 = X1 - X2;
	const int DX23 = X2 - X3;
	const int DX31 = X3 - X1;

	const int DY12 = Y1 - Y2;
	const int DY23 = Y2 - Y3;
	const int DY31 = Y3 - Y1;

	// Fixed-point deltas
	const int FDX12 = DX12 << 4;
	const int FDX23 = DX23 << 4;
	const int FDX31 = DX31 << 4;

	const int FDY12 = DY12 << 4;
	const int FDY23 = DY23 << 4;
	const int FDY31 = DY31 << 4;

	// Bounding rectangle
	int minx = (min(X1, X2, X3) + 0xF) >> 4;
	int maxx = (max(X1, X2, X3) + 0xF) >> 4;
	int miny = (min(Y1, Y2, Y3) + 0xF) >> 4;
	int maxy = (max(Y1, Y2, Y3) + 0xF) >> 4;

	// Block size, standard 8x8 (must be power of two)
	const int q = 8;

	// Start in corner of 8x8 block
	minx &= ~(q - 1);
	miny &= ~(q - 1);

	(char*&)color_buffer += miny * stride;

	// Half-edge constants
	int C1 = DY12 * X1 - DX12 * Y1;
	int C2 = DY23 * X2 - DX23 * Y2;
	int C3 = DY31 * X3 - DX31 * Y3;

	// Correct for fill convention
	if (DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
	if (DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
	if (DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

	// Loop through blocks
	for (int y = miny; y < maxy; y += q)
	{
		for (int x = minx; x < maxx; x += q)
		{
			// Corners of block
			int x0 = x << 4;
			int x1 = (x + q - 1) << 4;
			int y0 = y << 4;
			int y1 = (y + q - 1) << 4;

			// Evaluate half-space functions
			bool a00 = C1 + DX12 * y0 - DY12 * x0 > 0;
			bool a10 = C1 + DX12 * y0 - DY12 * x1 > 0;
			bool a01 = C1 + DX12 * y1 - DY12 * x0 > 0;
			bool a11 = C1 + DX12 * y1 - DY12 * x1 > 0;
			int a = (a00 << 0) | (a10 << 1) | (a01 << 2) | (a11 << 3);

			bool b00 = C2 + DX23 * y0 - DY23 * x0 > 0;
			bool b10 = C2 + DX23 * y0 - DY23 * x1 > 0;
			bool b01 = C2 + DX23 * y1 - DY23 * x0 > 0;
			bool b11 = C2 + DX23 * y1 - DY23 * x1 > 0;
			int b = (b00 << 0) | (b10 << 1) | (b01 << 2) | (b11 << 3);

			bool c00 = C3 + DX31 * y0 - DY31 * x0 > 0;
			bool c10 = C3 + DX31 * y0 - DY31 * x1 > 0;
			bool c01 = C3 + DX31 * y1 - DY31 * x0 > 0;
			bool c11 = C3 + DX31 * y1 - DY31 * x1 > 0;
			int c = (c00 << 0) | (c10 << 1) | (c01 << 2) | (c11 << 3);

			// Skip block when outside an edge
			if (a == 0x0 || b == 0x0 || c == 0x0) continue;

			unsigned * buffer = (unsigned*)color_buffer;

			// Accept whole block when totally covered
			if (a == 0xF && b == 0xF && c == 0xF)
			{
				for (int iy = 0; iy < q; iy++)
				{
					for (int ix = x; ix < x + q; ix++)
					{
						buffer[ix] = color;
					}

					(char*&)buffer += stride;
				}
			}
			else // Partially covered block
			{
				int CY1 = C1 + DX12 * y0 - DY12 * x0;
				int CY2 = C2 + DX23 * y0 - DY23 * x0;
				int CY3 = C3 + DX31 * y0 - DY31 * x0;

				for (int iy = y; iy < y + q; iy++)
				{
					int CX1 = CY1;
					int CX2 = CY2;
					int CX3 = CY3;

					for (int ix = x; ix < x + q; ix++)
					{
						if (CX1 > 0 && CX2 > 0 && CX3 > 0)
						{
							buffer[ix] = color;
						}

						CX1 -= FDY12;
						CX2 -= FDY23;
						CX3 -= FDY31;
					}

					CY1 += FDX12;
					CY2 += FDX23;
					CY3 += FDX31;

					(char*&)buffer += stride;
				}
			}
		}

		(char*&)color_buffer += q * stride;
	}
}
