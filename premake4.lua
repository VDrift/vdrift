solution "VDrift"
	project "vdrift"
		kind "WindowedApp"
		language "C++"
		location "build"
		targetdir "."
		includedirs {"src"}
		files {"src/**"}

	configurations {"Debug", "Release"}

	configuration "Release"
		defines {"NDEBUG"}
		flags {"OptimizeSpeed"}

	configuration "Debug"
		defines {"DEBUG"}
		flags {"ExtraWarnings", "Symbols"}

	configuration {"windows"}
		flags {"StaticRuntime"}
		includedirs {"win32/include", "win32/bullet"}
		libdirs {"win32/lib"}
		links {"opengl32", "glu32", "glew32", "SDLmain", "SDL", "wsock32", "ws2_32"}
		files {"win32/bullet/**.h", "win32/bullet/**.cpp"}
		links {"SDL_image", "SDL_gfx", "vorbisfile", "curl", "archive"}
		postbuildcommands {"xcopy /d /y /f win32\lib\*.dll"}

	configuration {"windows", "codeblocks"}
		links {"mingw32"}

	configuration {"linux"}
		includedirs {"/usr/local/include/bullet/", "/usr/include/bullet"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "LinearMath", "BulletDynamics", "BulletCollision", "GL", "GLU", "GLEW", "SDL", "vorbisfile", "SDL_image", "SDL_gfx"}

	configuration {"macosx"}
		files {"tools/osx/config_mac.mm", "tools/osx/SDLMain.h", "tools/osx/SDLMain.m"}
		includedirs { "tools/osx/", "tools/osx/Frameworks/Archive.framework/Headers", "tools/osx/Frameworks/BulletCollision.framework/Headers", "tools/osx/Frameworks/BulletDynamics.framework/Headers" }
		libdirs { "tools/osx/Frameworks" }
		links { "Cocoa.framework", "Vorbis.framework", "libcurl.framework", "SDL.framework", "SDL_image.framework", "SDL_gfx.framework", "Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "LinearMath.framework", "OpenGL.framework" }
		postbuildcommands {"cp -r ../tools/osx/Frameworks/ ../vdrift.app/Contents/Frameworks/"}
