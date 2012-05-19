#include "gui/guibutton.h"

#include <cassert>

GUIBUTTON::GUIBUTTON() :
	m_enabled(true),
	m_alpha(1)
{
	// ctor
}

GUIBUTTON::~GUIBUTTON()
{
	// dtor
}

void GUIBUTTON::SetAlpha(SCENENODE & scene, float value)
{
	m_alpha = value;
	float enabled = m_enabled ? 1.0 : 0.5;
	m_label.SetAlpha(scene, value * enabled);
}

void GUIBUTTON::SetVisible(SCENENODE & scene, bool value)
{
	m_label.SetVisible(scene, value);
}

bool GUIBUTTON::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	return m_enabled && m_label.InFocus(cursorx, cursory);
}

void GUIBUTTON::SetEnabled(SCENENODE & scene, bool value)
{
	m_enabled = value;
	m_label.SetAlpha(scene, m_alpha);
}

void GUIBUTTON::SetText(const std::string & value)
{
	m_label.SetText(value);
}

void GUIBUTTON::SetupDrawable(
	SCENENODE & scene,
	const FONT & font,
	int align,
	float scalex, float scaley,
	float x, float y,
	float w, float h, float z,
	float r, float g, float b)
{
	m_label.SetupDrawable(
		scene, font, align, scalex, scaley,
		x, y, w, h, z,
		r, g, b);
}
