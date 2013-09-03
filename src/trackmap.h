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

#ifndef _TRACKMAP_H
#define _TRACKMAP_H

#include "mathvector.h"
#include "graphics/scenenode.h"
#include "roadstrip.h"

#include <list>
#include <string>
#include <ostream>

class ContentManager;
class Texture;

class TrackMap
{
public:
	TrackMap();

	~TrackMap();

	///w and h are the display device dimensions in pixels.  returns true if successful.
	bool BuildMap(
		const std::list <RoadStrip> & roads,
		int w,
		int h,
		const std::string & trackname,
		const std::string & texturepath,
		ContentManager & content,
		std::ostream & error_output);

	void Unload();

	///update the map with provided information for map visibility, as well as a list of car positions and whether or not they're the player car
	void Update(bool mapvisible, const std::list <std::pair<Vec3, bool> > & carpositions);

	SceneNode & GetNode() {return mapnode;}

private:
	//track map size in real world
	Vec2 mapsize;
	float map_w_min, map_w_max;
	float map_h_min, map_h_max;
	float scale;
	const int MAP_WIDTH;
	const int MAP_HEIGHT;

	//screen size
	Vec2 screen;

	//position of the trackmap on screen
	Vec2 position;

	//size of the trackmap on screen
	Vec2 size;

	//size of the car dot on screen
	Vec2 dot_size;

	SceneNode mapnode;
	keyed_container <Drawable>::handle mapdraw;
	VertexArray mapverts;

	// car dot textures
	std::tr1::shared_ptr<Texture> cardot0;
	std::tr1::shared_ptr<Texture> cardot1;
	std::tr1::shared_ptr<Texture> cardot0_focused;
	std::tr1::shared_ptr<Texture> cardot1_focused;

	class CarDot
	{
		public:
			void Init(
				SceneNode & topnode,
				std::tr1::shared_ptr<Texture> tex,
				const Vec2 & corner1,
				const Vec2 & corner2)
			{
				dotdraw = topnode.GetDrawlist().twodim.insert(Drawable());
				Drawable & drawref = GetDrawable(topnode);
				drawref.SetVertArray(&dotverts);
				drawref.SetCull(false, false);
				drawref.SetColor(1,1,1,0.7);
				drawref.SetDrawOrder(0.1);
				Retexture(topnode, tex);
				Reposition(corner1, corner2);
			}
			void Retexture(SceneNode & topnode, std::tr1::shared_ptr<Texture> newtex)
			{
				assert(newtex.get());
				GetDrawable(topnode).SetDiffuseMap(newtex);
			}
			void Reposition(const Vec2 & corner1, const Vec2 & corner2)
			{
				dotverts.SetToBillboard(corner1[0], corner1[1], corner2[0], corner2[1]);
			}
			void SetVisible(SceneNode & topnode, bool visible)
			{
				GetDrawable(topnode).SetDrawEnable(visible);
			}
			void DebugPrint(SceneNode & topnode, std::ostream & out) const
			{
				const Drawable & drawref = GetDrawable(topnode);
				out << &drawref << ": enable=" << drawref.GetDrawEnable() << ", tex=" << drawref.GetDiffuseMap() << ", verts=" << drawref.GetVertArray() << std::endl;
			}
			keyed_container <Drawable>::handle & GetDrawableHandle()
			{
				return dotdraw;
			}

		private:
			keyed_container <Drawable>::handle dotdraw;
			VertexArray dotverts;

			Drawable & GetDrawable(SceneNode & topnode)
			{
				return topnode.GetDrawlist().twodim.get(dotdraw);
			}

			const Drawable & GetDrawable(SceneNode & topnode) const
			{
				return topnode.GetDrawlist().twodim.get(dotdraw);
			}
	};

	std::list <CarDot> dotlist;
};

#endif
