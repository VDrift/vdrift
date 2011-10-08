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
	
	platforms {"native", "universal"}

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
		prebuildcommands {"DATE=`date +%Y-%m-%d`\necho \"#ifndef _DEFINITIONS_H\" > $SRCROOT/src/definitions.h\necho \"#define _DEFINITIONS_H\" >> $SRCROOT/src/definitions.h\necho \"char* get_mac_data_dir();\" >> $SRCROOT/src/definitions.h\necho '#define SETTINGS_DIR \"Library/Preferences/VDrift\"' >> $SRCROOT/src/definitions.h\necho \"#define DATA_DIR get_mac_data_dir()\" >> $SRCROOT/src/definitions.h\necho '#define PACKAGE \"VDrift\"' >> $SRCROOT/src/definitions.h\necho '#define LOCALEDIR \"/usr/share/locale\"' >> $SRCROOT/src/definitions.h\necho \"#ifndef VERSION\" >> $SRCROOT/src/definitions.h\necho '#define VERSION \"\'$DATE\'\"' >> $SRCROOT/src/definitions.h\necho \"#endif //VERSION\" >> $SRCROOT/src/definitions.h\necho \"#ifndef REVISION\" >> $SRCROOT/src/definitions.h\necho '#define REVISION \"\'$DATE\'\"' >> $SRCROOT/src/definitions.h\necho \"#endif //REVISION\" >> $SRCROOT/src/definitions.h\necho \"#endif // _DEFINITIONS_H\" >> $SRCROOT/src/definitions.h"} --Generate definitions.h.
		files {"vdrift-mac/config_mac.mm", "vdrift-mac/SDLMain.h", "vdrift-mac/SDLMain.m", "vdrift-mac/Info.plist", "vdrift-mac/Readme.rtf", "vdrift-mac/icon.icns"} --Add mac specfic files to project.
		includedirs {"vdrift-mac", "vdrift-mac/Frameworks/Archive.framework/Headers", "vdrift-mac/Frameworks/BulletCollision.framework/Headers", "vdrift-mac/Frameworks/BulletDynamics.framework/Headers"} --Add paths to Header Search Paths (removing need for "ifdef __APPLE__"'s in source).
		libdirs {"vdrift-mac/Frameworks"} --Add Frameworks folder to Library Search Paths. We need to add it to Framework Search Paths instead. 
		links {"Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "libcurl.framework", "LinearMath.framework", "Ogg.framework", "SDL_gfx.framework", "SDL_image.framework", "SDL_net.framework", "SDL.framework", "Vorbis.framework", "AppKit.framework", "OpenGL.framework"} --Tell xcode to link to frameworks.
		postbuildcommands {'cp -r vdrift-mac/Frameworks/ "build/Debug Native/vdrift.app/Contents/Frameworks/"\n'} --Copy frameworks to app for portibility.
		postbuildcommands {'cd "$TARGET_BUILD_DIR"\n\n#Make the directory which our disk image will be made of.\nmkdir VDrift\n\n#Copy vdrift there.\n/Developer/Tools/CpMac -r "$TARGET_BUILD_DIR/VDrift.app" VDrift/\n\n#Copy readme.\n/Developer/Tools/CpMac $SRCROOT/vdrift-mac/ReadMe.rtf VDrift/ReadMe.rtf\n\n#Copy data and remove unnecessary stuff.\nmkdir VDrift/data\n/Developer/Tools/CpMac -r $SRCROOT/data/carparts VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/lists VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/music VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/settings VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/shaders VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/skins VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/textures VDrift/data/\n/Developer/Tools/CpMac -r $SRCROOT/data/trackparts VDrift/data/\nmkdir VDrift/data/cars\n/Developer/Tools/CpMac -r $SRCROOT/data/cars/F1-02 VDrift/data/cars/\n/Developer/Tools/CpMac -r $SRCROOT/data/cars/XS VDrift/data/cars/\nmkdir VDrift/data/tracks\n/Developer/Tools/CpMac -r $SRCROOT/data/tracks/paulricard88 VDrift/data/tracks/\n/Developer/Tools/CpMac -r $SRCROOT/data/tracks/weekend VDrift/data/tracks/\nfind VDrift/ -type f -name SConscript -exec rm {} ";"\nfind VDrift/ -type f -name \.DS_Store -exec rm -f {} ";"\nfind -d VDrift/ -type d -name \.svn -exec rm -rf {} ";"\n\n#Finally make a disk image out of the stuff.\nhdiutil create -srcfolder VDrift vdrift-mac-yyyy-mm-dd.dmg\nhdiutil internet-enable -yes vdrift-mac-yyyy-mm-dd.dmg\n\n#Cleanup.\nrm -r VDrift'} --Make a disk image with application, minimal data and readme.
