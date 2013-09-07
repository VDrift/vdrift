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

#ifndef _DYNAMICSDRAW_H
#define _DYNAMICSDRAW_H

#include "graphics/scenenode.h"
#include "graphics/vertexarray.h"
#include "LinearMath/btIDebugDraw.h"

class DynamicsDraw : public btIDebugDraw
{
public:
	DynamicsDraw();

	~DynamicsDraw();

	// add line segment to vertexarray, deferred drawing
	void drawLine(
		const btVector3& from,
		const btVector3& to,
		const btVector3& color);

	void drawContactPoint(
		const btVector3& pointOnB,
		const btVector3& normalOnB,
		btScalar distance,
		int lifeTime,
		const btVector3& color);

	void reportErrorWarning(const char* warningString);

	void draw3dText(const btVector3& location, const char* textString);

	void setDebugMode(int debugMode);

	int getDebugMode() const;

	SceneNode & getNode();

	// clear vertexarray after drawing
	void clear();

private:
	VertexArray m_contacts;
	VertexArray m_shapes;
	SceneNode m_node;
	int m_debugMode;
};

#endif // _DYNAMICSDRAW_H
