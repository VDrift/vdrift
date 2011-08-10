solution "VDrift"
	project "vdrift"
		kind "ConsoleApp"
		language "C++"
		location "."
		targetdir "."
		includedirs {"include", "src"}
		links {"SDL_image", "SDL_gfx", "vorbisfile", "curl", "archive"}
		files {"include/**.h", "src/**.cpp"}
	
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
		postbuildcommands {"xcopy /d /y /f win32\lib\*.dll"}
	
	configuration {"windows", "codeblocks"}
		links {"mingw32"}
		
	configuration {"linux"}
		libdirs {"/usr/X11R6/lib"}
        links {"GL", "GLU", "GLEW"}
		
	configuration {"macosx"}