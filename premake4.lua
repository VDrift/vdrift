write_definitions_h = function()
	local def = io.open("src/definitions.h", "w")
	def:write("#ifndef _DEFINITIONS_H\n")
	def:write("#define _DEFINITIONS_H\n")
	if _OPTIONS["datadir"] then
		def:write("#define DATA_DIR \"" .. _OPTIONS["datadir"] .. "\"\n")
	else
		def:write("#define DATA_DIR \"/usr/local/share/games/vdrift/data\"\n")
	end
	if _OPTIONS["force_feedback"] then
		def:write("#define ENABLE_FORCE_FEEDBACK\n")
	end
	if _OPTIONS["binreloc"] then
		def:write("#define ENABLE_BINRELOC\n")
	end
	def:write("#define VERSION \"development-full\"\n")
	def:write("#define REVISION \"latest\"\n")
	def:write("#endif // _DEFINITIONS_H\n")
end

solution "VDrift"
	project "vdrift"
		kind "WindowedApp"
		language "C++"
		location "build"
		targetdir "."
		includedirs {"src"}
		files {"src/**.h", "src/**.cpp"}
		write_definitions_h()

	configurations {"Debug", "Release"}

	configuration "Release"
		defines {"NDEBUG"}
		flags {"OptimizeSpeed"}

	configuration "Debug"
		defines {"DEBUG"}
		flags {"ExtraWarnings", "Symbols"}

	configuration {"windows", "codeblocks"}
		links {"mingw32"}
		
	configuration {"vs2008"}
		defines {"__PRETTY_FUNCTION__=__FUNCSIG__"}

	configuration {"windows"}
		flags {"StaticRuntime"}
		--debugdir "."
		includedirs {"vdrift-win/include", "vdrift-win/bullet"}
		libdirs {"vdrift-win/lib"}
		links {"opengl32", "glu32", "glew32", "SDLmain", "SDL", "SDL_image", "SDL_gfx", "vorbisfile", "curl", "archive-2", "wsock32", "ws2_32"}
		files {"vdrift-win/bullet/**.h", "vdrift-win/bullet/**.cpp"}
		postbuildcommands {"xcopy /d /y /f ..\\vdrift-win\\lib\\*.dll ..\\"}

	configuration {"linux"}
		includedirs {"/usr/local/include/bullet/", "/usr/include/bullet"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "BulletDynamics", "BulletCollision", "LinearMath", "GL", "GLU", "GLEW", "SDL", "vorbisfile", "SDL_image", "SDL_gfx"}

	configuration {"macosx"}
		files {"vdrift-mac/config_mac.mm", "vdrift-mac/SDLMain.h", "vdrift-mac/SDLMain.m"}
		includedirs { "vdrift-mac", "vdrift-mac/Frameworks/Archive.framework/Headers", "vdrift-mac/Frameworks/BulletCollision.framework/Headers", "vdrift-mac/Frameworks/BulletDynamics.framework/Headers" }
		libdirs { "vdrift-mac/Frameworks" }
		links { "Cocoa.framework", "Vorbis.framework", "libcurl.framework", "SDL.framework", "SDL_image.framework", "SDL_gfx.framework", "Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "LinearMath.framework", "OpenGL.framework" }
		postbuildcommands {"cp -r ../vdrift-mac/Frameworks/ ../vdrift.app/Contents/Frameworks/"}
		
	newoption {
		trigger = "datadir",
		value = "PATH",
		description = "Path suffix where where VDrift data will be installed",
	}

	newoption {
		trigger = "bindir",
		value = "PATH",
		description = "Path suffix where vdrift binary executable will be installed",
	}
	
	newoption {
		trigger = "force_feedback",
		description = "Enable force feedback"
	}
	
	newoption {
		trigger = "binreloc",
		description = "Compile with Binary Relocation support"
	}

	newaction {
		trigger = "install",
		description = "Install vdrift binary to bindir",
		execute = function ()
			if not _OPTIONS["bindir"] then
				print "No bindir specified."
			elseif  os.is("linux") then
				os.copyfile("vdrift", _OPTIONS["bindir"])
			elseif  os.is("windows") then
				print("Install binary into " .. _OPTIONS["bindir"])
				os.copyfile("vdrift.exe", _OPTIONS["bindir"])
				os.copyfile("*.dll", _OPTIONS["bindir"])
			else
				print "Configuration not supported"
			end
		end
	}
