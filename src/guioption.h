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

	void ReplaceValues(const std::list <std::pair<std::string, std::string> > & newvalues);

	void SetInfo(const std::string & newtext, const std::string & newdesc, const std::string & newtype);

	void Insert(const std::string & stored_value, const std::string & display_value);

	void SetMinMaxPercentage(float newmin, float newmax, bool newpercentage);

	/// returns true if the storedvaluename was found
	bool SetCurrentValue(const std::string & storedvaluename);

	/// increment the current_value to the next value in the values list
	void Increment();

	/// decrement the current_value to the previous value in the values list
	void Decrement();

	void SetToFirstValue();

	const std::string & GetCurrentDisplayValue() const;

	const std::string & GetCurrentStorageValue() const;

	const std::list <std::pair<std::string,std::string> > & GetValueList() const;

	const std::string & GetDescription() const {return description;};

	const std::string & GetText() const {return text;};

	float GetMin() const {return min;}

	float GetMax() const {return max;}

	bool GetPercentage() const {return percentage;}

	/// signal changed value to connected slots
	Signal1<const std::string &> signal_val;
	Signal1<const std::string &> signal_str;

private:
	bool current_valid;
	std::list <std::pair<std::string, std::string> >::const_iterator current_value;
	std::list <std::pair<std::string, std::string> > values; //the first element of the pair is the (sometimes numeric) stored value, while the second element is the display value.  sometimes they are the same.
	std::string non_value_data; //this is used when the values map is empty, such as for numeric settings
	std::string description;
	std::string text;
	std::string type;
	float min;
	float max;
	bool percentage;
	static const std::string null; // returned if current value is not valid

	void SignalValue();
};

#endif
