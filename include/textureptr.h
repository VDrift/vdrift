#ifndef _TEXTUREPTR_H
#define _TEXTUREPTR_H

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

class TEXTURE;
typedef std::tr1::shared_ptr<TEXTURE> TexturePtr;

#endif // _TEXTUREPTR_H
