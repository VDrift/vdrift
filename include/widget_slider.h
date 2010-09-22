#ifndef _WIDGET_SLIDER_H
#define _WIDGET_SLIDER_H

#include "widget.h"
#include "sprite2d.h"
#include "text_draw.h"
#include "mathvector.h"

class FONT;

class WIDGET_SLIDER : public WIDGET
{
public:
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual bool ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup);
	
	virtual void SetName(const std::string & newname);
	
	virtual std::string GetDescription() const;
	
	virtual void SetDescription(const std::string & newdesc);
	
	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);
		
	virtual void AddHook(WIDGET * other);
	
	void SetColor(SCENENODE & scene, float r, float g, float b);
	
	void SetSetting(const std::string & newsetting);
	
	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> wedgetex,
		std::tr1::shared_ptr<TEXTURE> cursortex,
		float x,
		float y,
		float nw,
		float nh,
		float newmin,
		float newmax,
		bool ispercentage,
		const std::string & newsetting,
  		FONT * font,
  		float fontscalex,
  		float fontscaley,
  		std::ostream & error_output,
  		int draworder = 1);

private:
	TEXT_DRAWABLE text;
	SPRITE2D wedge;
	SPRITE2D cursor;
	std::string name;
	std::string description;
	std::string setting;
	MATHVECTOR <float, 2> corner1;
	MATHVECTOR <float, 2> corner2;
	float min, max, current;
	bool percentage;
	float w, h;
	std::list <WIDGET *> hooks;
	
	void UpdateText(SCENENODE & scene);
	
	void SendMessage(SCENENODE & scene, const std::string & message) const;
};

#endif
