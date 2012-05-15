#ifndef _GUIWIDGET_H
#define _GUIWIDGET_H

#include "signalslot.h"
#include <map>
#include <string>
#include <ostream>

class SCENENODE;
class GUIOPTION;

class GUIWIDGET
{
public:
	/// true if x, y within widget rectangle
	virtual bool InFocus(float x, float y) const;

	/// scale widget alpha [0, 1]
	virtual void SetAlpha(SCENENODE & scene, float value);

	/// override visibility
	virtual void SetVisible(SCENENODE & scene, bool value);

	/// true if the mouse is within the widget
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	/// action associated with the widget
	virtual std::string GetAction() const;

	virtual void SetName(const std::string & newname);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	/// write/read associated guioption
	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);

	virtual bool GetCancel() const;

	/// update widget state
	virtual void Update(SCENENODE & scene, float dt);

	/// base destructor
	virtual ~GUIWIDGET();

protected:
	float m_xmin, m_ymin, m_xmax, m_ymax;
	float m_r, m_g, m_b, m_a;
	bool m_visible;
	bool m_update;

	GUIWIDGET();
};

inline bool GUIWIDGET::InFocus(float x, float y) const
{
	return x < m_xmax && x > m_xmin && y < m_ymax && y > m_ymin;
}

inline void GUIWIDGET::SetAlpha(SCENENODE & scene, float value)
{
	//GetDrawable(scene).SetColor(m_r, m_g, m_b, m_a * value);
}

inline void GUIWIDGET::SetVisible(SCENENODE & scene, bool value)
{
	//GetDrawable(scene).SetDrawEnable(m_visible & value);
}

inline bool GUIWIDGET::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	return false;
}

inline std::string GUIWIDGET::GetAction() const
{
	return "";
}

inline void GUIWIDGET::SetName(const std::string & newname)
{
	// optional
}

inline std::string GUIWIDGET::GetDescription() const
{
	return "";
}

inline void GUIWIDGET::SetDescription(const std::string & newdesc)
{
	// optional
}

inline void GUIWIDGET::UpdateOptions(
	SCENENODE & scene,
	bool save_to_options,
	std::map<std::string, GUIOPTION> & optionmap,
	std::ostream & error_output)
{
	// optional
}

inline bool GUIWIDGET::GetCancel() const
{
	return false;
}

inline void GUIWIDGET::Update(SCENENODE & scene, float dt)
{
	// optional
}

inline GUIWIDGET::~GUIWIDGET()
{
	// destructor
}

inline GUIWIDGET::GUIWIDGET() :
	m_xmin(0), m_ymin(0), m_xmax(0), m_ymax(0),
	m_r(1), m_g(1), m_b(1), m_a(0),
	m_visible(true),
	m_update(false)
{
	// constructor
}

#endif // _GUIH
