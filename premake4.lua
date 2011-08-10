solution "VDrift"
	project "vdrift"
		kind "WindowedApp"
		language "C++"
		location "."
		targetdir "."
		includedirs {"src"}
		files {"src/**"}
		location "build"

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
		includedirs {"/usr/local/include/bullet/"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "LinearMath", "BulletDynamics", "BulletCollision", "GL", "GLU", "GLEW", "SDL", "vorbisfile", "SDL_image", "SDL_gfx"}

	configuration {"macosx"}
