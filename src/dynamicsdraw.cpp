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

#include "dynamicsdraw.h"

DynamicsDraw::DynamicsDraw() :
	m_debugMode(DBG_NoDebug)
{
	Drawable drawable;
	drawable.SetLineSize(1.0);
	drawable.SetDrawEnable(true);

	drawable.SetColor(1, 0, 0);
	drawable.SetVertArray(&m_contacts);
	m_node.GetDrawlist().debug_lines.insert(drawable);

	drawable.SetColor(0, 0, 1);
	drawable.SetVertArray(&m_shapes);
	m_node.GetDrawlist().debug_lines.insert(drawable);
}

DynamicsDraw::~DynamicsDraw()
{
	// dtor
}

void DynamicsDraw::drawLine(
	const btVector3& from,
	const btVector3& to,
	const btVector3& color)
{
	int vcount = 6;
	float verts[6] = {from.x(), from.y(), from.z(), to.x(), to.y(), to.z()};
	int vsize;
	const float* vbase;
	m_shapes.GetVertices(vbase, vsize);
	m_shapes.SetVertices(verts, vcount, vsize);
}

void DynamicsDraw::drawContactPoint(
	const btVector3& pointOnB,
	const btVector3& normalOnB,
	btScalar distance,
	int lifeTime,
	const btVector3& color)
{
	btVector3 from = pointOnB;
	btVector3 to = pointOnB + normalOnB;

	int vcount = 6;
	float verts[6] = {from.x(), from.y(), from.z(), to.x(), to.y(), to.z()};
	int vsize;
	const float* vbase;
	m_contacts.GetVertices(vbase, vsize);
	m_contacts.SetVertices(verts, vcount, vsize);
}

void DynamicsDraw::reportErrorWarning(const char* warningString)
{
	// not implemented
}

void DynamicsDraw::draw3dText(const btVector3& location, const char* textString)
{
	// not implemented
}

void DynamicsDraw::setDebugMode(int debugMode)
{
	m_debugMode = debugMode;
}

int DynamicsDraw::getDebugMode() const
{
	return m_debugMode;
}

SceneNode & DynamicsDraw::getNode()
{
	return m_node;
}

void DynamicsDraw::clear()
{
	m_contacts.Clear();
	m_shapes.Clear();
}
