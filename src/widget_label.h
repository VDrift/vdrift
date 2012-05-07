#ifndef _WIDGET_LABEL_H
#define _WIDGET_LABEL_H

#include "widget.h"
#include "scenenode.h"
#include "text_draw.h"

class TEXTURE;
class FONT;

class WIDGET_LABEL : public WIDGET
{
public:
	WIDGET_LABEL();

	virtual ~WIDGET_LABEL();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		int align,
		float scalex, float scaley,
		float x, float y,
		float w, float h, float z,
		float r, float g, float b);

	void ReviseDrawable(SCENENODE & scene, const std::string & text);

	void SetText(SCENENODE & scene, const std::string & text);

	const std::string & GetText() const;

private:
	keyed_container<DRAWABLE>::handle m_draw;
	TEXT_DRAW m_text_draw;
	std::string m_text;
	const FONT * m_font;
	float m_x, m_y;
	float m_scalex, m_scaley;
	int m_align;

	DRAWABLE & GetDrawable(SCENENODE & scene);
};

#endif
