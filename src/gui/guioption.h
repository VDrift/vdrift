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

#ifndef _GUIOPTION_H
#define _GUIOPTION_H

#include "signalslot.h"

#include <map>
#include <string>
#include <vector>

class GuiOption
{
public:
	typedef std::vector< std::pair<std::string, std::string> > List;

	GuiOption();

	GuiOption(const GuiOption & other);

	GuiOption & operator=(const GuiOption & other);

	/// will move new value elements to option list
	void SetValues(const std::string & curvalue, const List & values);

	void SetInfo(
		const std::string & newdesc,
		const std::string & newtype,
		float newmin, float newmax,
		bool newpercent);

	/// reset to first value if passed value invalid
	void SetCurrentValue(const std::string & value);

	/// increment current_value to the next in the values list
	void Increment();

	/// decrement current_value to the previous in the values list
	void Decrement();

	/// set current value to nth in the value list
	void SetToNthValue(int n);

	void SetToFirstValue();

	const std::string & GetCurrentDisplayValue() const;

	const std::string & GetCurrentStorageValue() const;

	const List & GetValueList() const;

	const std::string & GetDescription() const {return m_description;};

	bool IsFloat() const { return m_type == type_float; };

	/// get value range, parameters are offset and value range vector
	Slot2<int, std::vector<std::string> &> get_val;
	Slot2<int, std::vector<std::string> &> get_str;

	/// signal values update, parameter is value count
	Signal1<const std::string &> signal_update;

	/// option signals (normalized value, value, string, index)
	Signal1<const std::string &> signal_valn;
	Signal1<const std::string &> signal_val;
	Signal1<const std::string &> signal_str;
	Signal1<const std::string &> signal_nth;

	/// option slots (normalized value, value, string, increment, decrement)
	Slot1<const std::string &> set_valn;
	Slot1<const std::string &> set_val;
	Slot1<int> set_nth;
	Slot0 set_prev;
	Slot0 set_next;

private:
	/// list option
	List m_values;
	size_t m_current_value;

	/// meta data
	std::string m_description;
	enum type {type_float, type_other} m_type;

	/// string/int/float option
	std::string m_data;
	float m_min;
	float m_max;
	bool m_percent;

	void SignalValue();

	void SetCurrentValueNorm(const std::string & value);

	void GetDisplayValues(int offset, std::vector<std::string> & vals);

	void GetStorageValues(int offset, std::vector<std::string> & vals);
};

#endif
