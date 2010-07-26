#ifndef _MODELPTR_H
#define _MODELPTR_H

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

class MODEL;
typedef std::tr1::shared_ptr<MODEL> ModelPtr;

#endif // _MODELPTR_H
