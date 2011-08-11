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

	configuration {"windows", "codeblocks"}
		links {"mingw32"}

	configuration {"windows"}
		flags {"StaticRuntime"}
		includedirs {"vdrift-win/include", "vdrift-win/bullet"}
		libdirs {"vdrift-win/lib"}
		links {"opengl32", "glu32", "glew32", "SDLmain", "SDL", "SDL_image", "SDL_gfx", "vorbisfile", "curl", "archive-2", "wsock32", "ws2_32"}
		files {"vdrift-win/bullet/**"}
		postbuildcommands {"xcopy /d /y /f ..\\vdrift-win\\lib\\*.dll ..\\"}

	configuration {"linux"}
		includedirs {"/usr/local/include/bullet/", "/usr/include/bullet"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "LinearMath", "BulletDynamics", "BulletCollision", "GL", "GLU", "GLEW", "SDL", "vorbisfile", "SDL_image", "SDL_gfx"}

	configuration {"macosx"}
		files {"vdrift-mac/config_mac.mm", "vdrift-mac/SDLMain.h", "vdrift-mac/SDLMain.m"}
		includedirs { "vdrift-mac", "vdrift-mac/Frameworks/Archive.framework/Headers", "vdrift-mac/Frameworks/BulletCollision.framework/Headers", "vdrift-mac/Frameworks/BulletDynamics.framework/Headers" }
		libdirs { "vdrift-mac/Frameworks" }
		links { "Cocoa.framework", "Vorbis.framework", "libcurl.framework", "SDL.framework", "SDL_image.framework", "SDL_gfx.framework", "Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "LinearMath.framework", "OpenGL.framework" }
		postbuildcommands {"cp -r ../vdrift-mac/Frameworks/ ../vdrift.app/Contents/Frameworks/"}
