update_options = function()
	local options = {}
	if not options["settings"] then options["settings"]=".vdrift" end
	if not options["bindir"] then options["bindir"]="/usr/local/bin" end
	if not options["datadir"] then options["datadir"]="/usr/local/share/games/vdrift/data" end
	local f = io.open("vdrift.cfg", "r")
	if f then
		for line in f:lines() do
			key_value = string.explode(line, "=")
			options[key_value[1]] = key_value[2]
		end
		f:close()
	end
	for key, value in pairs(options) do
		if (not _OPTIONS[key]) or (_OPTIONS[key] == "") then
			_OPTIONS[key] = value
		end
	end
	f = io.open("vdrift.cfg", "w")
	for key, value in pairs(_OPTIONS) do
		if value then
			f:write(key .. "=" .. value .. "\n")
		end
	end
	f:close()
end

write_definitions_h = function()
	local f = io.open("src/definitions.h", "w")
	f:write("#ifndef _DEFINITIONS_H\n")
	f:write("#define _DEFINITIONS_H\n")
	f:write("#define SETTINGS_DIR \"" .. _OPTIONS["settings"] .. "\"\n")
	f:write("#define DATA_DIR \"" .. _OPTIONS["datadir"] .. "\"\n")
	if _OPTIONS["force_feedback"] then
		f:write("#define ENABLE_FORCE_FEEDBACK\n")
	end
	if _OPTIONS["binreloc"] then
		f:write("#define ENABLE_BINRELOC\n")
	end
	f:write("#define VERSION \"development-full\"\n")
	f:write("#define REVISION \"latest\"\n")
	f:write("#endif // _DEFINITIONS_H\n")
	f:close()
end

newoption {
	trigger = "settings",
	value = "PATH",
	description = "Directory in user\'s home dir where settings will be stored."
}

newoption {
	trigger = "datadir",
	value = "PATH",
	description = "Path where where VDrift data will be installed."
}

newoption {
	trigger = "bindir",
	value = "PATH",
	description = "Path where VDrift executable will be installed."
}

newoption {
	trigger = "force_feedback",
	description = "Enable force feedback."
}

newoption {
	trigger = "binreloc",
	description = "Compile with Binary Relocation support."
}

newaction {
	trigger = "install",
	description = "Install vdrift binary to bindir",
	execute = function ()
		local _binname = iif(os.is("windows"), "vdrift.exe", "vdrift")
		if not os.isfile(_binname) then
			print "VDrift binary not found. Build vdrift before install."
			return
		end

		local _datadir = _OPTIONS["datadir"]
		local _bindir = _OPTIONS["bindir"]
		if not _bindir then
			_bindir = bindir
			print("Using default installation directory: " .. _bindir)
			print("To set installation directory use: premake4 -bindir=PATH install.")
		end

		print("Install binary into " .. _bindir)
		if not os.isdir(_bindir) then
			print("Create " .. _bindir)
			if not os.mkdir(_bindir) then
				print("Failed to create " .. _bindir)
			end
		end

		os.copyfile(_binname, _bindir .. "/")
	end
}

solution "VDrift"
	project "vdrift"
		kind "WindowedApp"
		language "C++"
		location "build"
		targetdir "."
		includedirs {"src"}
		files {"src/**.h", "src/**.cpp"}
		update_options()
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
