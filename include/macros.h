#ifndef _MACROS_H
#define _MACROS_H

#define _SERIALIZE_(ser,varname) if (!ser.Serialize(#varname,varname)) return false

#endif
