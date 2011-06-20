#ifndef _SIMPLETREE_H
#define _SIMPLETREE_H

#include <map>
#include <iostream>

template<typename KEYTYPE, typename VALUETYPE>
class SIMPLETREE
{
private:
	void DebugPrint(int level, std::ostream & mystream) const
	{
		mystream << value << endl;

		for (typename std::map <KEYTYPE,SIMPLETREE>::iterator i = branch.begin();
				   i != branch.end(); i++)
		{
			for (int n = 0; n < level; n++)
				mystream << "-";

			mystream << i->first << "=";
			i->second.DebugPrint(level+1, mystream);
		}
	}

public:
	SIMPLETREE() : parent(NULL) {}

	SIMPLETREE * parent;
	VALUETYPE value;
	std::map <KEYTYPE, SIMPLETREE> branch;

	void DebugPrint() const {DebugPrint(0,cout);}
};


#endif
