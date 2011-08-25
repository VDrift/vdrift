update_options = function()
	local options = {}
	options["settings"]=".vdrift"
	options["bindir"]="/usr/local/bin"
	options["datadir"]="/usr/local/share/games/vdrift/data"
	options["force_feedback"]="no"
	options["binreloc"]="no"
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
		if options[key] then
			f:write(key.."="..value.."\n")
		end
	end
	f:close()
end

write_definitions_h = function()
	local f = io.open("src/definitions.h", "w")
	f:write("#ifndef _DEFINITIONS_H\n")
	f:write("#define _DEFINITIONS_H\n")
	f:write("#define SETTINGS_DIR \"".._OPTIONS["settings"].."\"\n")
	f:write("#define DATA_DIR \"".._OPTIONS["datadir"].."\"\n")
	if _OPTIONS["force_feedback"] == "yes" then
		f:write("#define ENABLE_FORCE_FEEDBACK\n")
	end
	if _OPTIONS["binreloc"] == "yes" then
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
	value = "VALUE",
	description = "Enable force feedback.",
	allowed = {{"yes", "Enable option"}, {"no", "Disable option"}}
}

newoption {
	trigger = "binreloc",
	value = "VALUE",
	description = "Compile with Binary Relocation support.",
	allowed = {{"yes", "Enable option"}, {"no", "Disable option"}}
}

newaction {
	trigger = "install",
	description = "Install vdrift binary to bindir.",
	execute = function ()
		local binname = iif(os.is("windows"), "vdrift.exe", "vdrift")
		if not os.isfile(binname) then
			print "VDrift binary not found. Build vdrift before install."
			return
		end
		local bindir = _OPTIONS["bindir"]
		print("Install binary into "..bindir)
		if not os.isdir(bindir) then
			print("Create "..bindir)
			if not os.mkdir(bindir) then
				print("Failed to create "..bindir)
				return
			end
		end
		os.copyfile(os.getcwd().."/"..binname, bindir.."/"..binname)
	end
}

newaction {
	trigger = "install-data",
	description = "Install vdrift data to datadir.",
	execute = function ()
		local cwd = os.getcwd()
		local sourcedir = "data"
		local targetdir = _OPTIONS["datadir"]
		if not os.isdir(sourcedir) then
			print "VDrift data not found in current working directory."
			return
		end
		local dirlist = os.matchdirs(sourcedir.."/**")
		for i, val in ipairs(dirlist) do
			os.mkdir(targetdir..val:sub(5))
		end
		local filelist = os.matchfiles(sourcedir.."/**")
		for i, val in ipairs(filelist) do
			if not val:find("SConscript", 1, true) then
				os.copyfile(cwd.."/"..val, targetdir..val:sub(5))
			end
		end
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

	configuration {"vs*"}
		defines {"__PRETTY_FUNCTION__=__FUNCSIG__", "_USE_MATH_DEFINES", "NOMINMAX"}
		buildoptions {"/wd4100", "/wd4127", "/wd4244", "/wd4245", "/wd4305", "/wd4512", "/wd4800"}
	
	configuration {"vs*", "Debug"}
		linkoptions {"/NODEFAULTLIB:\"msvcrt.lib\""}

	configuration {"windows"}
		location "."
		includedirs {"vdrift-win/include", "vdrift-win/bullet"}
		libdirs {"vdrift-win/lib"}
		links {"opengl32", "glu32", "glew32", "SDLmain", "SDL", "SDL_image", "SDL_gfx", "vorbisfile", "libcurl", "libarchive-2", "wsock32", "ws2_32"}
		files {"vdrift-win/bullet/**.h", "vdrift-win/bullet/**.cpp"}
		postbuildcommands {"xcopy /d /y /f .\\vdrift-win\\lib\\*.dll .\\"}

	configuration {"linux"}
		includedirs {"/usr/local/include/bullet/", "/usr/include/bullet"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "BulletDynamics", "BulletCollision", "LinearMath", "GL", "GLU", "GLEW", "SDL", "vorbisfile", "SDL_image", "SDL_gfx"}

	configuration {"macosx"}
		files {"vdrift-mac/config_mac.mm", "vdrift-mac/SDLMain.h", "vdrift-mac/SDLMain.m"}
		includedirs { "vdrift-mac", "vdrift-mac/Frameworks/Archive.framework/Headers", "vdrift-mac/Frameworks/BulletCollision.framework/Headers", "vdrift-mac/Frameworks/BulletDynamics.framework/Headers" }
		libdirs { "vdrift-mac/Frameworks" }
		links { "Cocoa.framework", "Vorbis.framework", "libcurl.framework", "SDL.framework", "SDL_image.framework", "SDL_gfx.framework", "Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "LinearMath.framework", "OpenGL.framework" }
		postbuildcommands {"cp -r ./vdrift-mac/Frameworks/ ./vdrift.app/Contents/Frameworks/"}
