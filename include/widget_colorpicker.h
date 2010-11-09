#ifndef _WIDGET_COLORPICKER_H
#define _WIDGET_COLORPICKER_H

#include "widget.h"
#include "sprite2d.h"

class WIDGET_COLORPICKER : public WIDGET
{
public:
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual void SetName(const std::string & newname);
	
	virtual void SetDescription(const std::string & newdesc);
	
	virtual std::string GetDescription() const;
	
	virtual void AddHook(WIDGET * other);
	
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx,
		float cursory,
		bool cursordown,
		bool cursorjustup);
	
	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);
	
	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> cursortex,
		std::tr1::shared_ptr<TEXTURE> htex,
		std::tr1::shared_ptr<TEXTURE> svtex,
		std::tr1::shared_ptr<TEXTURE> bgtex,
		float x, float y, float w, float h,
		const std::string & setting,
		std::ostream & error_output,
		int draworder);
	
private:
	SPRITE2D sv_plane;
	SPRITE2D sv_bg;
	SPRITE2D h_bar;
	SPRITE2D sv_cursor;
	SPRITE2D h_cursor;
	std::string name;
	std::string description;
	std::string setting;
	MATHVECTOR <float, 2> min;
	MATHVECTOR <float, 2> max;
	MATHVECTOR <float, 3> rgb;
	MATHVECTOR <float, 3> hsv;
	float margin;
	float csize;
	std::list <WIDGET *> hooks;
	
	void SendMessage(SCENENODE & scene, const std::string & message) const;
};

#endif // _WIDGET_COLORPICKER_H
