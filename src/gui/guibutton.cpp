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
	float basealpha = m_enabled ? 1.0 : 0.5;
	GUILABEL::SetAlpha(scene, value * basealpha);
	m_alpha = value;
}

std::string GUIBUTTON::GetDescription() const
{
	return m_description;
}

void GUIBUTTON::SetDescription(const std::string & value)
{
	m_description = value;
}

bool GUIBUTTON::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	if (!m_enabled || !InFocus(cursorx, cursory))
		return false;

	if (cursorjustup)
		signal_action();

	return true;
}

void GUIBUTTON::SetEnabled(SCENENODE & scene, bool value)
{
	m_enabled = value;
	SetAlpha(scene, m_alpha);
}
