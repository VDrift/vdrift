/*
    SDL_image:  An example image loading library for use with SDL
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* A simple library to load images of various formats as SDL surfaces */

#ifndef _SDL_IMAGE_H
#define _SDL_IMAGE_H

#include "SDL/SDL.h"
#include "SDL/SDL_version.h"
#include "SDL/begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Printable format: "%d.%d.%d", MAJOR, MINOR, PATCHLEVEL
*/
#define SDL_IMAGE_MAJOR_VERSION	1
#define SDL_IMAGE_MINOR_VERSION	2
#define SDL_IMAGE_PATCHLEVEL	10

/* This macro can be used to fill a version structure with the compile-time
 * version of the SDL_image library.
 */
#define SDL_IMAGE_VERSION(X)						\
{									\
	(X)->major = SDL_IMAGE_MAJOR_VERSION;				\
	(X)->minor = SDL_IMAGE_MINOR_VERSION;				\
	(X)->patch = SDL_IMAGE_PATCHLEVEL;				\
}

/* This function gets the version of the dynamically linked SDL_image library.
   it should NOT be used to fill a version structure, instead you should
   use the SDL_IMAGE_VERSION() macro.
 */
extern DECLSPEC const SDL_version * SDLCALL IMG_Linked_Version(void);

typedef enum
{
    IMG_INIT_JPG = 0x00000001,
    IMG_INIT_PNG = 0x00000002,
    IMG_INIT_TIF = 0x00000004
} IMG_InitFlags;

/* Loads dynamic libraries and prepares them for use.  Flags should be
   one or more flags from IMG_InitFlags OR'd together.
   It returns the flags successfully initialized, or 0 on failure.
 */
extern DECLSPEC int SDLCALL IMG_Init(int flags);

/* Unloads libraries loaded with IMG_Init */
extern DECLSPEC void SDLCALL IMG_Quit(void);

/* Load an image from an SDL data source.
   The 'type' may be one of: "BMP", "GIF", "PNG", etc.

   If the image format supports a transparent pixel, SDL will set the
   colorkey for the surface.  You can enable RLE acceleration on the
   surface afterwards by calling:
	SDL_SetColorKey(image, SDL_RLEACCEL, image->format->colorkey);
 */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTyped_RW(SDL_RWops *src, int freesrc, char *type);
/* Convenience functions */
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load(const char *file);
extern DECLSPEC SDL_Surface * SDLCALL IMG_Load_RW(SDL_RWops *src, int freesrc);

/* Invert the alpha of a surface for use with OpenGL
   This function is now a no-op, and only provided for backwards compatibility.
*/
extern DECLSPEC int SDLCALL IMG_InvertAlpha(int on);

/* Functions to detect a file type, given a seekable source */
extern DECLSPEC int SDLCALL IMG_isICO(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isCUR(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isBMP(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isGIF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isJPG(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isLBM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPCX(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPNG(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isPNM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isTIF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXCF(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXPM(SDL_RWops *src);
extern DECLSPEC int SDLCALL IMG_isXV(SDL_RWops *src);

/* Individual loading functions */
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadICO_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadCUR_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadBMP_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadGIF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadJPG_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadLBM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPCX_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPNG_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadPNM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTGA_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadTIF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXCF_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXPM_RW(SDL_RWops *src);
extern DECLSPEC SDL_Surface * SDLCALL IMG_LoadXV_RW(SDL_RWops *src);

extern DECLSPEC SDL_Surface * SDLCALL IMG_ReadXPMFromArray(char **xpm);

/* We'll use SDL for reporting errors */
#define IMG_SetError	SDL_SetError
#define IMG_GetError	SDL_GetError

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "SDL/close_code.h"

#endif /* _SDL_IMAGE_H */
