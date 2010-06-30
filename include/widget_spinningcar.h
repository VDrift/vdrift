#ifndef _WIDGET_SPINNINGCAR_H
#define _WIDGET_SPINNINGCAR_H

#include "widget.h"
#include "scenenode.h"
#include "mathvector.h"

class CAR;
class TEXTUREMANAGER;

class WIDGET_SPINNINGCAR : public WIDGET
{
public:
	WIDGET_SPINNINGCAR();
	
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual void HookMessage(SCENENODE & scene, const std::string & message, const std::string & from);
	
	virtual void Update(SCENENODE & scene, float dt);
	
	void SetupDrawable(
		SCENENODE & scene,
		const std::string & texturesize,
		const std::string & datapath,
		float x,
		float y,
		const MATHVECTOR <float, 3> & newcarpos,
		TEXTUREMANAGER & textures,
		std::ostream & error_output,
		int order = 0);
	
private:
	std::string data;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 3> carpos;
	int draworder;
	std::ostream * errptr;
	float rotation;
	std::string carname;
	std::string carpaint;
	bool wasvisible;
	float r, g, b;
	
	keyed_container <SCENENODE>::handle carnode;
	TEXTUREMANAGER * textures;
	
	std::list <CAR> car; ///< only ever one element, please
	
	SCENENODE & GetCarNode(SCENENODE & parent);
	
	void SetColor(SCENENODE & scene, float r, float g, float b);
	
	void Unload(SCENENODE & parent);
	
	void Load(SCENENODE & parent);
};

#endif // _WIDGET_SPINNINGCAR_H
