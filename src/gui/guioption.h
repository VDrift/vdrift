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

	GuiOption() {};

	/// will move new value elements to option list
	void SetValues(const std::string & curvalue, const List & values);

	void SetInfo(
		const std::string & newdesc,
		const std::string & newtype,
		float newmin, float newmax,
		bool newpercent);

	/// reset to first value if passed value invalid
	void SetCurrentValue(const std::string & value);

	/// only valid for floating point options
	void SetCurrentValueNorm(const std::string & value);

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

	void GetDisplayValues(int offset, std::vector<std::string> & vals);

	void GetStorageValues(int offset, std::vector<std::string> & vals);

	const std::string & GetDescription() const {return m_description;};

	bool IsFloat() const { return m_float; };

	/// signal values update, parameter is value count
	Signald<const std::string &> signal_update;

	/// option signals (normalized value, value, string, index)
	Signald<const std::string &> signal_valn;
	Signald<const std::string &> signal_val;
	Signald<const std::string &> signal_str;
	Signald<const std::string &> signal_nth;

private:
	/// list option
	List m_values;
	size_t m_current_value = 0;

	/// meta data
	std::string m_description;

	/// string/int/float option
	std::string m_data;
	float m_min = 0;
	float m_max = 0;
	bool m_float = true;
	bool m_percent = false;

	void SignalValue();
};

#endif
