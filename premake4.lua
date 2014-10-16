update_options = function()
	local options = {}
	options["settings"]=".vdrift"
	options["bindir"]="/usr/local/bin"
	options["datadir"]="/usr/local/share/games/vdrift/data"
	options["localedir"]="/usr/share/locale"
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

gen_definitions_h = function()
	update_options()
	local f = io.open("src/definitions.h", "w")
	f:write("#ifndef _DEFINITIONS_H\n")
	f:write("#define _DEFINITIONS_H\n")
	f:write("#define SETTINGS_DIR \"".._OPTIONS["settings"].."\"\n")
	f:write("#define DATA_DIR \"".._OPTIONS["datadir"].."\"\n")
	f:write("#define LOCALE_DIR \"".._OPTIONS["localedir"].."\"\n")
	if _OPTIONS["binreloc"] == "yes" then
		f:write("#define ENABLE_BINRELOC\n")
	end
	f:write("#define VERSION \"development\"\n")
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
	trigger = "localedir",
	value = "PATH",
	description = "Path where where VDrift locale will be installed."
}

newoption {
	trigger = "bindir",
	value = "PATH",
	description = "Path where VDrift executable will be installed."
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
		linkoptions {"-static-libstdc++", "-static-libgcc"}

	configuration {"vs*"}
		defines {"__PRETTY_FUNCTION__=__FUNCSIG__", "_USE_MATH_DEFINES", "NOMINMAX"}
		buildoptions {"/wd4100", "/wd4127", "/wd4244", "/wd4245", "/wd4305", "/wd4355", "/wd4512", "/wd4800"}

	configuration {"vs*", "Debug"}
		linkoptions {"/NODEFAULTLIB:\"msvcrt.lib\""}

	configuration {"windows"}
		_OPTIONS["datadir"] = "./data"
		_OPTIONS["localedir"] = "./data/locale"
		gen_definitions_h()
		location "."
		defines {"HAVE_LIBC"} --SDL2
		includedirs {"vdrift-win/include", "vdrift-win/bullet"}
		libdirs {"vdrift-win/lib"}
		links {"opengl32", "SDL2main", "SDL2", "SDL2_image", "vorbisfile", "iconv2", "intl", "curl", "wsock32", "ws2_32"}
		files {"vdrift-win/bullet/**.h", "vdrift-win/bullet/**.cpp"}
		postbuildcommands {"xcopy /d /y /f .\\vdrift-win\\lib\\*.dll .\\"}

	configuration {"linux"}
		gen_definitions_h()
		includedirs {"/usr/local/include/bullet/", "/usr/include/bullet"}
		libdirs {"/usr/X11R6/lib"}
		links {"archive", "curl", "vorbisfile", "BulletDynamics", "BulletCollision", "LinearMath", "GL", "GLU", "GLEW", "SDL", "SDL_image"}

	configuration {"macosx"}
		prebuildcommands {'if [ -f "SRCROOT"/../src/definitions.h ]; then\n    rm "SRCROOT"/../src/definitions.h\nfi\nDATE=`date +%Y-%m-%d`\necho "#ifndef _DEFINITIONS_H" > "$SRCROOT"/../src/definitions.h\necho "#define _DEFINITIONS_H" >> "$SRCROOT"/../src/definitions.h\necho "char* get_mac_data_dir();" >> "$SRCROOT"/../src/definitions.h\necho "#define SETTINGS_DIR \"Library/Preferences/VDrift\"" >> "$SRCROOT"/../src/definitions.h\necho "#define DATA_DIR get_mac_data_dir()" >> "$SRCROOT"/../src/definitions.h\necho "#define PACKAGE \"VDrift\"" >> "$SRCROOT"/../src/definitions.h\necho "#define LOCALEDIR \"/usr/share/locale\"" >> "$SRCROOT"/../src/definitions.h\necho "#ifndef VERSION" >> "$SRCROOT"/../src/definitions.h\necho "#define VERSION \"$DATE\"" >> "$SRCROOT"/../src/definitions.h\necho "#endif //VERSION" >> "$SRCROOT"/../src/definitions.h\necho "#ifndef REVISION" >> "$SRCROOT"/../src/definitions.h\necho "#define REVISION \"$DATE\"" >> "$SRCROOT"/../src/definitions.h  #No longer have svn revision to fetch, and can\'t get git, so use date at the moment.\necho "#endif //REVISION" >> "$SRCROOT"/../src/definitions.h\necho "#endif // _DEFINITIONS_H" >> "$SRCROOT"/../src/definitions.h\n'} --Generate definitions.h.
		files {"vdrift-mac/config_mac.mm", "vdrift-mac/SDLMain.h", "vdrift-mac/SDLMain.m", "vdrift-mac/Info.plist", "vdrift-mac/Readme.rtfd", "vdrift-mac/License.rtf", "vdrift-mac/icon.icns", "vdrift-mac/VDrift.entitlements"} --Add mac specfic files to project.
		includedirs {".", "../src", "Frameworks/Archive.framework/Headers", "Frameworks/BulletCollision.framework/Headers", "Frameworks/BulletDynamics.framework/Headers", "Frameworks/BulletSoftBody.framework/Headers", "Frameworks/cURL.framework/Headers", "Frameworks/GLEW.framework/Headers", "Frameworks/LinearMath.framework/Headers", "Frameworks/Ogg.framework/Headers", "Frameworks/SDL_image.framework/Headers", "Frameworks/SDL.framework/Headers", "Frameworks/Vorbis.framework/Headers"} --Add paths to Header Search Paths (removing need for "ifdef __APPLE__"'s in source).
		libdirs {"vdrift-mac/Frameworks"} --Add Frameworks folder to Library Search Paths. We need to add it to Framework Search Paths instead.
		links {"Archive.framework", "BulletCollision.framework", "BulletDynamics.framework", "BulletSoftBody.framework", "GLEW.framework", "cURL.framework", "LinearMath.framework", "Ogg.framework", "SDL_image.framework", "SDL.framework", "Vorbis.framework", "AppKit.framework", "OpenGL.framework"} --Tell Xcode to link to frameworks.
		postbuildcommands {'cp -r vdrift-mac/Frameworks/ "$TARGET_BUILD_DIR/VDrift.app/Contents/Frameworks/"\n'} --Copy frameworks to app for portibility.
		postbuildcommands {'#Change to the build directory.\ncd "$TARGET_BUILD_DIR"\n\n#Remove any previously copied data.\nif [ -d VDrift.app/Contents/Resources/data ]; then\n    rm -r VDrift.app/Contents/Resources/data\nfi\n\n#Could be a broken alias too.\nif [ -f VDrift.app/Contents/Resources/data ]; then\n    rm VDrift.app/Contents/Resources/data\nfi\n\n#Only copy some data, and do it tidily, if we\'re releasing.\nif [ "${CONFIGURATION}" == "Release" ]; then\n\n    #Copy data and remove unnecessary files.\n    mkdir VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/carparts VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/lists VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/music VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/settings VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/shaders VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/skins VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/textures VDrift.app/Contents/Resources/data\n    cp -r "$SRCROOT"/../data/trackparts VDrift.app/Contents/Resources/data\n\n    mkdir VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/350Z VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/360 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/ATT VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/CO VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/CS VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/F1-02 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/G4 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/LE VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/M7 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/MC VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/MI VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/SV VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/T73 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/TC6 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/TL2 VDrift.app/Contents/Resources/data/cars\n    cp -r "$SRCROOT"/../data/cars/XS VDrift.app/Contents/Resources/data/cars\n\n    mkdir VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/bahrain VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/estoril88 VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/jerez88 VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/lemans VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/monaco88 VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/monza88 VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/paulricard88 VDrift.app/Contents/Resources/data/tracks\n    cp -r "$SRCROOT"/../data/tracks/rouen VDrift.app/Contents/Resources/data/tracks\n\n    find VDrift.app/Contents/Resources/data -type f -name SConscript -exec rm {} ';'\n    find VDrift.app/Contents/Resources/data -type f -name \.DS_Store -exec rm -f {} ';'\n    find -d VDrift.app/Contents/Resources/data -type d -name \.svn -exec rm -rf {} ';'\n\nelse\n    #Copy all data.\n    cp -r "$SRCROOT"/../data VDrift.app/Contents/Resources\nfi\n'} --Full or minimal data into application.
