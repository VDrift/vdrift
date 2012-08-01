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

#include "loaddrawable.h"
#include "content/contentmanager.h"
#include "graphics/textureinfo.h"
#include "cfg/ptree.h"

LoadDrawable::LoadDrawable(
	const std::string & path,
	const int anisotropy,
	ContentManager & content,
	std::list<std::tr1::shared_ptr<MODEL> > & modellist,
	std::ostream & error) :
	path(path),
	anisotropy(anisotropy),
	content(content),
	modellist(modellist),
	error(error)
{
	// ctor
}

bool LoadDrawable::operator()(
	const PTree & cfg,
	SCENENODE & topnode,
	keyed_container<SCENENODE>::handle * nodehandle,
	keyed_container<DRAWABLE>::handle * drawhandle)
{
	std::vector<std::string> texname;
	if (!cfg.get("texture", texname)) return true;

	std::string meshname;
	if (!cfg.get("mesh", meshname, error)) return false;

	return operator()(meshname, texname, cfg, topnode, nodehandle, drawhandle);
}

bool LoadDrawable::operator()(
	const std::string & meshname,
	const std::vector<std::string> & texname,
	const PTree & cfg,
	SCENENODE & topnode,
	keyed_container<SCENENODE>::handle * nodeptr,
	keyed_container<DRAWABLE>::handle * drawptr)
{
	DRAWABLE drawable;

	// set textures
	TEXTUREINFO texinfo;
	texinfo.mipmap = true;
	texinfo.anisotropy = anisotropy;
	std::tr1::shared_ptr<TEXTURE> tex;
	if (texname.size() == 0)
	{
		error << "No texture defined" << std::endl;
		return false;
	}
	if (texname.size() > 0)
	{
		content.load(tex, path, texname[0], texinfo);
		drawable.SetDiffuseMap(tex);
	}
	if (texname.size() > 1)
	{
		content.load(tex, path, texname[1], texinfo);
		drawable.SetMiscMap1(tex);
	}
	if (texname.size() > 2)
	{
		content.load(tex, path, texname[2], texinfo);
		drawable.SetMiscMap2(tex);
	}

	// set mesh
	std::tr1::shared_ptr<MODEL> mesh;
	content.load(mesh, path, meshname);

	std::string scalestr;
	if (cfg.get("scale", scalestr) &&
		!content.get(mesh, path, meshname + scalestr))
	{
		MATHVECTOR<float, 3> scale;
		std::stringstream s(scalestr);
		s >> scale;

		VERTEXARRAY meshva = mesh->GetVertexArray();
		meshva.Scale(scale[0], scale[1], scale[2]);
		content.load(mesh, path, meshname + scalestr, meshva);
	}
	drawable.SetModel(*mesh);
	modellist.push_back(mesh);

	// set color
	MATHVECTOR<float, 4> col(1);
	if (cfg.get("color", col))
	{
		drawable.SetColor(col[0], col[1], col[2], col[3]);
	}

	// set node
	SCENENODE * node = &topnode;
	if (nodeptr != 0)
	{
		if (!nodeptr->valid())
		{
			*nodeptr = topnode.AddNode();
			assert(nodeptr->valid());
		}
		node = &topnode.GetNode(*nodeptr);
	}

	MATHVECTOR<float, 3> pos, rot;
	if (cfg.get("position", pos) | cfg.get("rotation", rot))
	{
		if (node == &topnode)
		{
			// position relative to parent, create child node
			keyed_container <SCENENODE>::handle nodehandle = topnode.AddNode();
			node = &topnode.GetNode(nodehandle);
		}
		node->GetTransform().SetTranslation(pos);
		node->GetTransform().SetRotation(QUATERNION<float>(rot[0]/180*M_PI, rot[1]/180*M_PI, rot[2]/180*M_PI));
	}

	// set drawable
	keyed_container<DRAWABLE>::handle drawtemp;
	keyed_container<DRAWABLE>::handle * draw = &drawtemp;
	if (drawptr != 0) draw = drawptr;

	std::string drawtype;
	if (cfg.get("draw", drawtype))
	{
		if (drawtype == "emissive")
		{
			drawable.SetDecal(true);
			*draw = node->GetDrawlist().lights_emissive.insert(drawable);
		}
		else if (drawtype == "transparent")
		{
			*draw = node->GetDrawlist().normal_blend.insert(drawable);
		}
	}
	else
	{
		*draw = node->GetDrawlist().car_noblend.insert(drawable);
	}

	return true;
}
