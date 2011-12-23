#include "dynamicsdraw.h"

DynamicsDraw::DynamicsDraw() :
	m_debugMode(DBG_NoDebug)
{
	DRAWABLE drawable;
	drawable.SetLineSize(1.0);
	drawable.SetDrawEnable(true);

	drawable.SetColor(1, 0, 0);
	drawable.SetVertArray(&m_contacts);
	m_node.GetDrawlist().normal_noblend_nolighting.insert(drawable);

	drawable.SetColor(0, 0, 1);
	drawable.SetVertArray(&m_shapes);
	m_node.GetDrawlist().normal_noblend_nolighting.insert(drawable);
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

SCENENODE & DynamicsDraw::getNode()
{
	return m_node;
}

void DynamicsDraw::clear()
{
	m_contacts.Clear();
	m_shapes.Clear();
}
