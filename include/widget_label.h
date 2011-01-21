#ifndef _WIDGET_LABEL_H
#define _WIDGET_LABEL_H

#include "widget.h"
#include "scenenode.h"
#include "text_draw.h"

#include <string>

class TEXTURE;
class FONT;

class WIDGET_LABEL : public WIDGET
{
public:
	WIDGET_LABEL();
	
	~WIDGET_LABEL() {};
	
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		const std::string & text,
		float x, float y,
		float scalex, float scaley,
		float nr, float ng, float nb,
		float z = 0,
		bool centered = true);
	
	void ReviseDrawable(SCENENODE & scene, const std::string & text);
	
	float GetWidth(const FONT & font, const std::string & text, float scale) const;
	
	void SetText(SCENENODE & scene, const std::string & text);
	
	const std::string & GetText();
	
private:
	TEXT_DRAW text_draw;
	keyed_container <DRAWABLE>::handle draw;
	const FONT * savedfont;
	float r, g, b;
	float saved_x, saved_y, saved_scalex, saved_scaley;
	bool saved_centered;
	
	DRAWABLE & GetDrawable(SCENENODE & scene)
	{
		return scene.GetDrawlist().text.get(draw);
	}
};

#endif
