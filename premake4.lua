settingsdir = ".vdrift"
bindir = "/usr/local/bin"
datadir = "/usr/local/share/games/vdrift/data"

newoption {
	trigger = "settings",
	value = "PATH",
	description = "Directory in user\'s home dir where settings will be stored. Default: " .. settingsdir,
}

newoption {
	trigger = "datadir",
	value = "PATH",
	description = "Path where where VDrift data will be installed. Default: " .. datadir,
}

newoption {
	trigger = "bindir",
	value = "PATH",
	description = "Path where VDrift executable will be installed. Default: " .. bindir,
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
		local _bindir = _OPTIONS["bindir"]
		local _binname = "vdrift" 
		
		if not _bindir then
			_bindir = bindir
			print("Using default installation directory: " .. _bindir)
			print("To set installation directory use: premake4 -bindir=PATH install.")
		end
		
		if os.is("windows") then 
			_binname = "vdrift.exe"
		end

		if not os.isfile(_binname) then
			print "VDrift binary not found. Build vdrift before install."
		else
			print("Install binary into " .. _bindir)
			if not os.isdir(_bindir) then
				print("Create " .. _bindir)
				if not os.mkdir(_bindir) then
					print("Failed to create " .. _bindir)
				end
			end
			os.copyfile(_binname, "E:/vdri")
		end
	end
}

write_definitions_h = function()
	local _datadir = iif(_OPTIONS["datadir"], _OPTIONS["datadir"], datadir)
	local _settings = iif(_OPTIONS["settings"], _OPTIONS["settings"], settingsdir)
	local def = io.open("src/definitions.h", "w")
	def:write("#ifndef _DEFINITIONS_H\n")
	def:write("#define _DEFINITIONS_H\n")
	def:write("#define SETTINGS_DIR \"" .. _settings .. "\"\n")
	def:write("#define DATA_DIR \"" .. _datadir .. "\"\n")
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
