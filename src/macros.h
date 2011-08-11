#ifndef _MACROS_H
#define _MACROS_H

#define _SERIALIZE_(ser,varname) if (!ser.Serialize(#varname,varname)) return false
#define _SERIALIZEENUM_(ser,varname,type) if (ser.GetIODirection() == joeserialize::Serializer::DIRECTION_INPUT) {int _enumint(0);if (!ser.Serialize(#varname,_enumint)) return false;varname=(type)_enumint;} else {int _enumint = varname;if (!ser.Serialize(#varname,_enumint)) return false;}

///break up the input into a vector of strings using the token characters given
std::vector <std::string> Tokenize(const std::string & input, const std::string & tokens);

#endif
