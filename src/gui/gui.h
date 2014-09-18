/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _GUI_H
#define _GUI_H

#include "guipage.h"
#include "guioption.h"
#include "guilanguage.h"
#include "font.h"

class Gui
{
public:
	Gui();

	const std::string & GetActivePageName() const;

	const std::string & GetLastPageName() const;

	SceneNode & GetNode();

	GuiPage & GetPage(const std::string & name);

	bool Active() const;

	bool GetInGame() const;

	void SetInGame(bool value);

	bool Load(
		const std::list <std::string> & pagelist,
		const std::map<std::string, GuiOption::List> & valuelists,
		const std::string & datapath,
		const std::string & optionsfile,
		const std::string & skinname,
		const std::string & language,
		const float screenhwratio,
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
		const GuiOption::List & newvalues,
		std::ostream & error_output);

	/// returns false if the specified page/label does not exist
	bool SetLabelText(const std::string & page, const std::string & label, const std::string & text);
	bool GetLabelText(const std::string & page, const std::string & label, std::string & text_output);

	/// iterate trough all pages and update labels, slow
	void SetLabelText(const std::string & page, const std::map<std::string, std::string> & label_text);
	void SetLabelText(const std::map<std::string, std::string> & label_text);

	/// access options
	std::string GetOptionValue(const std::string & name) const;
	void SetOptionValue(const std::string & name, const std::string & value);
	GuiOption & GetOption(const std::string & name);

	/// access to language dict and font for translation purposes
	const GuiLanguage & GetLanguageDict() const;
	const Font & GetFont() const;

	typedef std::map<std::string, GuiOption> OptionMap;
	typedef std::map<std::string, GuiPage> PageMap;

private:
	OptionMap options;
	PageMap pages;
	PageMap::iterator last_active_page;
	PageMap::iterator active_page;
	PageMap::iterator next_active_page;
	SceneNode node;
	GuiLanguage lang;
	Font font;
	float m_cursorx, m_cursory;			///< cache cursor position
	float animation_counter;
	float animation_count_start;
	float next_animation_count_start;
	bool ingame;

	/// page activate callback
	struct PageCb
	{
		Gui * gui;
		std::string page;
		Slot0 action;

		PageCb();
		PageCb(const PageCb & other);
		PageCb & operator=(const PageCb & other);
		void call();
	};
	std::vector<PageCb> page_activate;

	/// return false on failure
	/// on empty page name will attempt to load prev(last active) page
	bool ActivatePage(
		const std::string & pagename,
		float activation_time);

	/// add option slots to action map
	void RegisterOptions(
		StrSignalMap & vsignalmap,
		StrVecSlotlMap & vnactionmap,
		StrSlotMap & vactionmap,
		IntSlotMap & nactionmap,
		SlotMap & actionmap);
};

#endif
