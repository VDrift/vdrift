#include "widget_button.h"

#include <cassert>

WIDGET_BUTTON::WIDGET_BUTTON() :
	m_cancel(true),
	m_enabled(true),
	m_alpha(1)
{
	// ctor
}

WIDGET_BUTTON::~WIDGET_BUTTON()
{
	// dtor
}

void WIDGET_BUTTON::SetAlpha(SCENENODE & scene, float value)
{
	float basealpha = m_enabled ? 1.0 : 0.5;
	WIDGET_LABEL::SetAlpha(scene, value * basealpha);
	m_alpha = value;
}

std::string WIDGET_BUTTON::GetAction() const
{
	return m_active_action;
}

std::string WIDGET_BUTTON::GetDescription() const
{
	return m_description;
}

void WIDGET_BUTTON::SetDescription(const std::string & value)
{
	m_description = value;
}

bool WIDGET_BUTTON::GetCancel() const
{
	return m_cancel;
}

bool WIDGET_BUTTON::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	if (!m_enabled)
		return false;

	m_active_action.clear();

	if (!InFocus(cursorx, cursory))
		return false;

	if (cursorjustup)
		m_active_action = m_action;

	return true;
}

void WIDGET_BUTTON::SetCancel(bool value)
{
	m_cancel = value;
}

void WIDGET_BUTTON::SetAction(const std::string & value)
{
	m_action = value;
}

void WIDGET_BUTTON::SetEnabled(SCENENODE & scene, bool value)
{
	m_enabled = value;
	SetAlpha(scene, m_alpha);
}
