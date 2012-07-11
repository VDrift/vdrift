#ifndef _GUI_H
#define _GUI_H

#include "gui/guipage.h"
#include "gui/guioption.h"
#include "font.h"

#include <map>
#include <list>
#include <string>
#include <iostream>

class GUI
{
public:
	GUI();

	const std::string & GetActivePageName() const;

	const std::string & GetLastPageName() const;

	SCENENODE & GetNode();

	SCENENODE & GetPageNode(const std::string & name);

	GUIPAGE & GetPage(const std::string & name);

	bool Active() const;

	bool GetInGame() const;

	void SetInGame(bool value);

	bool Load(
		const std::list <std::string> & pagelist,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
		const std::string & datapath,
		const std::string & optionsfile,
		const std::string & menupath,
		const std::string & languagedir,
		const std::string & language,
		const std::string & texpath,
		const std::string & texsize,
		const float screenhwratio,
		const std::map <std::string, FONT> & fonts,
		std::map <std::string, Slot0*> actionmap,
		ContentManager & content,
		std::ostream & info_output,
		std::ostream & error_output);

    /// Clears out all variables and reset the class to what it looked like when it was originally initialized.
    /// Can be called whether the GUI is currently loaded or not.
    void Unload();

	void Deactivate();

	void ActivatePage(
		const std::string & pagename,
		float activation_time,
		std::ostream & error_output);

	/// movedown and moveup are true when the user has navigated up or down with the keyboard
	/// or joystick, while the cursor variables are set for mouse navigation.
	/// returns a list of actions for processing by the game.
	void ProcessInput(
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup,
		bool moveleft, bool moveright,
		bool moveup, bool movedown,
		bool select, bool cancel);

	void Update(float dt);

	/// if settings_are_newer is true, then this function will revise its internal options
	/// to match the settings passed in.  otherwise, it'll operate the other way around
	void GetOptions(std::map <std::string, std::string> & options) const;
	void SetOptions(const std::map <std::string, std::string> & options);

	void SetOptionValues(
		const std::string & optionname,
		const std::string & curvalue,
		std::list <std::pair <std::string, std::string> > & newvalues,
		std::ostream & error_output);

	/// returns false if the specified page/label does not exist
	bool SetLabelText(const std::string & page, const std::string & label, const std::string & text);
	bool GetLabelText(const std::string & page, const std::string & label, std::string & text_output);

	/// iterate trough all pages and update labels, slow
	void SetLabelText(const std::string & page, const std::map<std::string, std::string> & label_text);
	void SetLabelText(const std::map<std::string, std::string> & label_text);

	/// returns false if the specified page/label does not exist
	bool SetButtonEnabled(const std::string & page, const std::string & button, bool enable);

	/// access options
	std::string GetOptionValue(const std::string & name) const;
	void SetOptionValue(const std::string & name, const std::string & value);
	GUIOPTION & GetOption(const std::string & name);

private:
	typedef std::map<std::string, GUIOPTION> OPTIONMAP;
	typedef std::map<std::string, GUIPAGE> PAGEMAP;
	OPTIONMAP options;
	PAGEMAP pages;
	PAGEMAP::iterator last_active_page;
	PAGEMAP::iterator active_page;
	SCENENODE node;
	FONT font;
	float m_cursorx, m_cursory;			///< cache cursor position
	float animation_counter;
	float animation_count_start;
	bool ingame;

	struct PAGECB
	{
		GUI * gui;
		std::string page;
		Slot0 action;

		PAGECB();
		PAGECB(const PAGECB & other);
		PAGECB & operator=(const PAGECB & other);
		void call();
	};
	std::vector<PAGECB> page_activate;

	/// return false on failure
	bool ActivatePage(
		const std::string & pagename,
		float activation_time);

	/// return false on failure
	bool LoadOptions(
		const std::string & optionfile,
		const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists,
		const std::map<std::string, std::string> & languagemap,
		std::ostream & error_output);

	/// add option slots to action map
	void RegisterOptions(VSIGNALMAP & vsignalmap, VACTIONMAP & vactionmap, ACTIONMAP & actionmap);
};

#endif
