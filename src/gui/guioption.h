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
#include <list>

class GUIOPTION
{
public:
	GUIOPTION();

	GUIOPTION(const GUIOPTION & other);

	GUIOPTION & operator=(const GUIOPTION & other);

	/// will move new value elements to option list
	void SetValues(const std::string & curvalue, std::list <std::pair<std::string, std::string> > & newvalues);

	void SetInfo(const std::string & newdesc, const std::string & newtype);

	void SetMinMaxPercentage(float newmin, float newmax, bool newpercent);

	/// reset to first value if passed value invalid
	void SetCurrentValue(const std::string & value);

	/// increment the current_value to the next value in the values list
	void Increment();

	/// decrement the current_value to the previous value in the values list
	void Decrement();

	void SetToFirstValue();

	const std::string & GetCurrentDisplayValue() const;

	const std::string & GetCurrentStorageValue() const;

	const std::list <std::pair<std::string,std::string> > & GetValueList() const;

	const std::string & GetDescription() const {return description;};

	float GetMin() const {return min;}

	float GetMax() const {return max;}

	/// option signals (normalized value, value, string)
	Signal1<const std::string &> signal_valn;
	Signal1<const std::string &> signal_val;
	Signal1<const std::string &> signal_str;

	/// option slots (normalized value, value, string, increment, decrement)
	Slot1<const std::string &> set_valn;
	Slot1<const std::string &> set_val;
	Slot0 prev_val;
	Slot0 next_val;

private:
	/// list option
	std::list <std::pair<std::string, std::string> >::iterator current_value;
	std::list <std::pair<std::string, std::string> > values; //the first element of the pair is the (sometimes numeric) stored value, while the second element is the display value.  sometimes they are the same.
	
	/// meta data
	std::string description;
	enum type {type_float, type_other} type;

	/// string/int/float option
	std::string non_value_data;
	float min;
	float max;
	bool percent;

	void SignalValue();
	void SetCurrentValueNorm(const std::string & value);
};

#endif
