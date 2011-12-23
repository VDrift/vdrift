#ifndef _DYNAMICSDRAW_H
#define _DYNAMICSDRAW_H

#include "LinearMath/btIDebugDraw.h"
#include "scenenode.h"

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

	SCENENODE & getNode();

	// clear vertexarray after drawing
	void clear();

private:
	VERTEXARRAY m_contacts;
	VERTEXARRAY m_shapes;
	SCENENODE m_node;
	int m_debugMode;
};

#endif // _DYNAMICSDRAW_H
