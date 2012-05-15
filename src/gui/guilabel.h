#ifndef _GUILABEL_H
#define _GUILABEL_H

#include "gui/guiwidget.h"
#include "scenenode.h"
#include "text_draw.h"

class TEXTURE;
class FONT;

class GUILABEL : public GUIWIDGET
{
public:
	GUILABEL();

	virtual ~GUILABEL();

	virtual void SetAlpha(SCENENODE & scene, float value);

	virtual void SetVisible(SCENENODE & scene, bool value);

	// align: -1 left, 0 center, +1 right
	void SetupDrawable(
		SCENENODE & scene,
		const FONT & font,
		int align,
		float scalex, float scaley,
		float centerx, float centery,
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
