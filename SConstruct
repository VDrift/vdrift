import os, sys, time, errno, SCons

#---------------#
# Build Options #
#---------------#
opts = Variables('vdrift.conf', ARGUMENTS)
opts.Add('arch', 'Target architecture to compile vdrift for (x86, 686, p4, axp, a64, prescott, nocona, core2)', 'x86')
opts.Add('pkg_config', 'Executable for pkg-config', 'pkg-config')
opts.Add('destdir', 'Staging area to install VDrift to.  Useful for packagers. ', '')
opts.Add('builddir_release', 'Release build directory.', 'build')
opts.Add('builddir_debug', 'Debug build directory.', 'build')
opts.Add(BoolVariable('minimal', 'Only install minimal data (3 cars and 2 tracks)', 0))
opts.Add(BoolVariable('cache', 'Cache options in vdrift.conf', 1))
opts.Add(BoolVariable('release', 'Turn off debug option during build', 0))
opts.Add(BoolVariable('use_apbuild', 'Set this to 1 if you want to compile with apgcc to create an autopackage', 0))
opts.Add(BoolVariable('use_binreloc', 'Set this to 1 if you want to compile with Binary Relocation support', 1))
opts.Add(BoolVariable('os_cc', 'Set this to 1 if you want to use the operating system\'s C compiler environment variable.', 0))
opts.Add(BoolVariable('os_cxx', 'Set this to 1 if you want to use the operating system\'s C++ compiler environment variable.', 0))
opts.Add(BoolVariable('os_cxxflags', 'Set this to 1 if you want to use the operating system\'s C++ compiler flags environment variable.', 0))
opts.Add(BoolVariable('use_distcc', 'Set this to 1 to enable distributed compilation', 0))
opts.Add(BoolVariable('profiling', 'Turn on profiling output', 0))
opts.Add(BoolVariable('efficiency', 'Turn on compile-time efficiency warnings', 0))
opts.Add(BoolVariable('verbose', 'Show verbose compiling output', 1)) 

#--------------------------#
# Create Build Environment #
#--------------------------#
# define a list of CPPDEFINES so they don't get mangled...
cppdefines = []
default_settingsdir = ".vdrift"
default_prefix = "/usr/local"
default_datadir = "share/games/vdrift/data"
default_localedir = "share/locale"
default_bindir = "bin"

#---------------#
# FreeBSD build #
#---------------#
if sys.platform in ['freebsd6', 'freebsd7', 'freebsd8', 'freebsd9', 'freebsd10']:
    if 'LOCALBASE' in os.environ:
        LOCALBASE = os.environ['LOCALBASE']
    else:
        LOCALBASE = '/usr/local/'
    env = Environment(ENV = os.environ,
        CPPPATH = ['#src',LOCALBASE + '/include',LOCALBASE + '/include/bullet'],
        LIBPATH = ['.', '#lib', LOCALBASE + '/lib'],
        LINKFLAGS = ['-pthread','-lintl'],
        options = opts)
    check_headers = ['GL/gl.h', 'SDL2/SDL.h', 'SDL2/SDL_image.h', 'vorbis/vorbisfile.h', 'bullet/btBulletCollisionCommon.h']
    check_libs = []
    if 'CC' in os.environ:
        env.Replace(CC = os.environ['CC'])
    else:
        env.Replace(CC = "gcc")
    if 'CXX' in os.environ:
        env.Replace(CXX = os.environ['CXX'])
    else:
        env.Replace(CC = "g++")
    if 'CXXFLAGS' in os.environ:
        env.Append(CXXFLAGS = os.environ['CXXFLAGS'])

#------------#
# OS X build #
#------------#
elif sys.platform == 'darwin':
    opts.Add( ListVariable('universal', 
            'the target architectures to include in a universal binary.', 
            'none', ['ppc', 'i386']))
    opts.Add('SDK', 'the path to an SDK directory', '')

    env = Environment(ENV = os.environ,
        CPPPATH = ['#src', '#vdrift-mac/Frameworks', '#vdrift-mac/Frameworks/SDL2.framework/Headers', '#vdrift-mac/Libraries'],
        CCFLAGS = ['-std=c++14', '-Wall', '-Wextra'],
        CXXFLAGS = Split("$CCFLAGS -Wno-non-virtual-dtor -Wunused-parameter"),
        LIBPATH = ['.'],
        FRAMEWORKPATH = ['vdrift-mac/Frameworks/'],
        FRAMEWORKS = [ 'OpenGL' ],
        options = opts)

    # Setup universal binary support
    sdkfile = 'SDKSettings.plist'
    sdk_path = None
    
    if env['SDK']:
        sdk_path = FindFile( sdkfile, env['SDK'] )
    else:
        # check some reasonable locations
        sdk_path = FindFile( sdkfile,
            [ '/Developer/SDKs/MacOSX%s.sdk' % x for x in 
                [ '10.5', '10.4u' ] ] ) 

    for a in env['universal']:
        if not sdk_path:
            print('Building a universal binary require access to an ' + \
                'SDK that has universal \nbinary support.If you know ' + \
                'the location of such an SDK, specify it using the \n"SDK" option')
            Exit(1)
        env.Append( CCFLAGS = ['-arch', a],  LINKFLAGS = ['-arch', a] )

    if (len(env['universal']) and sdk_path):
        from os.path import dirname
        sdk_path = dirname( str( sdk_path ) ) 
        env.Append( CCFLAGS = ['-isysroot', sdk_path], 
            LINKFLAGS = ['-Wl,-syslibroot,%s' % sdk_path] )

    # Configure reasonable defaults
    default_settingsdir = 'Library/Preferences/VDrift'
    default_prefix = "/Applications/VDrift"
    default_datadir = "data"
    default_bindir = ""

    check_headers = ['OpenGL/gl.h', 'SDL2/sdl.h']
    check_libs = []
    cppdefines.append(("_DEFINE_OSX_HELPERS"))

#---------------#
# Windows build #
#---------------#
elif sys.platform in ['win32', 'msys', 'cygwin']:
    env = Environment(ENV = os.environ, tools = ['mingw'],
        CCFLAGS = ['-std=c++14', '-Wall', '-Wextra', '-mwindows'],
        CPPPATH = ['#src', '#vdrift-win/include', '#vdrift-win/bullet'],
        LIBPATH = ['#vdrift-win/dll'],
        #LINKFLAGS = ['-static-libgcc', '-static-libstdc++'],
        CPPDEFINES = ['_USE_MATH_DEFINES'],
        CC = 'gcc', CXX = 'g++',
        options = opts)
    check_headers = []
    check_libs = []

#-------------#
# POSIX build #
#-------------#
else:
    env = Environment(ENV = os.environ,
        CPPPATH = ['#src'],
        CCFLAGS = ['-std=c++14', '-Wall', '-Wextra'],#, '-pthread'],
        LIBPATH = ['.', '#lib'],
        #LINKFLAGS = ['-pthread'],
        CC = 'gcc', CXX = 'g++',
        options = opts)
    # Take environment variables into account
    if 'CXX' in os.environ:
        env['CXX'] = os.environ['CXX']
    if 'CXXFLAGS' in os.environ:
        env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
    if 'LDFLAGS' in os.environ:
        env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])
    check_headers = ['GL/gl.h', 'SDL2/SDL.h', 'SDL2/SDL_image.h', 'vorbis/vorbisfile.h', 'curl/curl.h', 'bullet/btBulletCollisionCommon.h', 'bullet/btBulletDynamicsCommon.h']
    check_libs = []

if ARGUMENTS.get('verbose') != "1":
       env['ARCOMSTR'] = "\tARCH $TARGET"
       env['CCCOMSTR'] = "\tCC $TARGET"
       env['CXXCOMSTR'] = "\tCPP $TARGET"
       env['LINKCOMSTR'] = "\tLINK $TARGET"
       env['RANLIBCOMSTR'] = "\tRANLIB $TARGET"

#-------------------------------#
# General configurarion options #
#-------------------------------#
opts.Add('settings', 'Directory under user\'s home dir where settings will be stored', default_settingsdir )
opts.Add('prefix', 'Path prefix.', default_prefix)
# in most case datadir doesn't exsist => do not use PathOption (Fails on build)
opts.Add('datadir', 'Path suffix where where VDrift data will be installed', default_datadir) 
opts.Add('localedir', 'Path where VDrift locale will be installed', default_localedir)
opts.Add('bindir', 'Path suffix where VDrift binary executable will be installed', default_bindir)

# For the OSX package, but could be useful otherwise
env['EXECUTABLE_NAME'] = 'vdrift'
env['PRODUCT_NAME'] = 'vdrift'

#--------------#
# Save Options #
#--------------#
opts.Update(env)
if env['cache']:
    opts.Save('vdrift.conf', env)

#--------------------------------------------#
# distcopy and tarballer functions           #
# by Paul Davis <paul@linuxaudiosystems.com> #
#--------------------------------------------#
def distfile( src_root, file ):
    path = os.path.normpath( file.path )
    if ( len( src_root ) and path.startswith( src_root ) ):
        path = path[ len( src_root ) + 1 : ]

    return path

def distemit (target, source, env ):
    treedir = str ( target[0] ) 
    src_root = ''
    if ( len( target ) > 1 ): src_root = target[1].path
    target = [ os.path.join( treedir, distfile( src_root, f ) ) 
                for f in source ]

    # Links of all types must be omitted because scons doesn't
    # know how to handle them.  It can't check for existance
    # or traverse links to directories and who knows what else
    # jul. 27 2007
    for t,s in zip( target, source ):
        if os.path.islink( str( t ) ):
            if os.path.islink( str( s ) ):
                target.remove( t )
                source.remove( s )
            else:
                env.Execute( Delete( t ) )

    return ( target, source )

def distcopy (target, source, env):
    # converted from system wrapper to pure python function
    # to better support windows

    for t, s in zip( target, source ):
        t = os.path.dirname( str( t ) )
        if not os.path.exists( t ): 
            env.Execute( Mkdir( t ) )
        s = str( s )
        if ( os.path.islink( s ) and 
                not os.path.isabs( os.readlink( s ) ) ):
            os.symlink( os.readlink( s ), 
                        os.path.join( t, os.path.basename( s ) ) )
        else:
            env.Execute( Copy( t,  s ) )


    return

def tarballer (target, source, env):            
    cmd = 'tar -jcf "%s" -C "%s" .'  % ( str(target[0]), str(source[0]) )
    #cmd = 'tar -jcf ' + str (target[0]) +  ' ' + str(source[0]) + "  --exclude '*~' "
    print('running ', cmd, ' ... ')
    p = os.popen (cmd)
    return p.close ()

#----------------------------------#
# definitions file writer function #
#----------------------------------#
def write_definitions(cppdefs=[]):
    defs_file = open("src/definitions.h", "w")
    defs_file.write("#ifndef _DEFINITIONS_H\n")
    defs_file.write("#define _DEFINITIONS_H\n")
    for item in cppdefs:
        if len(item) == 2:
            defs_file.write("#define " + item[0] + " " + item[1] + "\n")
        else:
            defs_file.write("#define " + item + "\n")
    defs_file.write("#endif // _DEFINITIONS_H\n")
    defs_file.close()

dist_bld = Builder ( action = distcopy,
                    emitter = distemit,
                    target_factory = SCons.Node.FS.default_fs.Entry,
                    source_factory = SCons.Node.FS.default_fs.Entry,
                    multi = 1)

tarball_bld = Builder (action = tarballer,
                       target_factory = SCons.Node.FS.default_fs.Entry,
                       source_factory = SCons.Node.FS.default_fs.Entry)

env.Append (BUILDERS = {'Distribute' : dist_bld})
env.Append (BUILDERS = {'Tarball' : tarball_bld})

#--------------------------------------------------#
# Builder for exporting an svn directory           #
#--------------------------------------------------#
def exportvisit( srcfiles, dirname, entries):
    from os.path import join, isdir, islink
    
    srcfiles.extend( [ File( join( dirname, e ) ) for e in entries 
            if not isdir( join( dirname, e ) ) ] )
    srcfiles.extend( [ Dir( join( dirname, e ) ) for e in entries
            if islink( join(dirname, e ) ) and isdir( join( dirname, e ) ) ] )

def exportemit (target, source, env ):
    source_str = str( source[0] )
    target.append( Dir( os.path.dirname( source_str ) ) )
    if ( os.path.isdir( source_str ) ):
        source = []
        os.path.walk( source_str, exportvisit, source )

    return distemit( target, source, env )


export_bld = Builder( action = distcopy,
                      emitter = exportemit,
                      target_factory = Entry,
                      source_factory = Entry,
                      multi = 0 )
env.Append (BUILDERS = {'WorkingExport' : export_bld })

#--------------------------------------------------#
# Builder for untarring something into another dir #
#--------------------------------------------------#
def tarcopy(target, source, env):
    cmd = 'tar zxf ' + source + ' -C ' + target
    p = os.popen(cmd)
    return p.close()

copy_tar_dir = Builder (action = tarcopy,
                        target_factory = SCons.Node.FS.default_fs.Entry,
                        source_factory = SCons.Node.FS.default_fs.Entry)

env.Append (BUILDERS = {'TarCopy' : copy_tar_dir})

#------------------------------------------------------------------------#
# Provide a utility for applying environment variables to template files #
#------------------------------------------------------------------------#
def process_template(target, source, env):
    for tgt, src in zip(  target, source ):
        file( str( tgt ), "w+" ).write( 
            env.subst( file( str( src ) ).read(), raw=1 ) )

env.Append (BUILDERS = 
    {'ProcessTemplate': Builder(action = process_template ) })

#------------#
# Mo Builder #
#------------#
def mo_builder(target, source, env):
    args = ['msgfmt', '-c', '-o', target[0].get_path(), source[0].get_path()]
    return os.spawnvp (os.P_WAIT, 'msgfmt', args)

env.Append (BUILDERS = {'MoBuild' : Builder(action = mo_builder)})

#------------#
# Build Help #
#------------#
Help("""
Type: 'scons' to compile with the default options.
      'scons arch=axp' to compile for Athlon XP support (other options: a64, 686, p4, x86, prescott, nocona, core2)
      'scons prefix=/usr/local' to install everything in another prefix.
      'scons destdir=$PWD/tmp' to install to $PWD/tmp staging area.
      'scons datadir=' to install data files into an alternate directory.
      'scons bindir=games/bin' to install executable into an alternate directory.
      'scons localedir=/usr/share/locale' to install language files into an alternate directory.
      'scons release=1' to turn on compiler optimizations and disable debugging info.
      'scons builddir_release=build' to set release build directory.
      'scons builddir_debug=build' to set debug build directory.
      'scons settings=.VDrift' to change settings directory.
      'scons install' (as root) to install VDrift.
      'scons wrapper' to build the Python wrapper used by track editor
      'scons use_apbuild=1' to create an autopackage (building with apbuild)
      'scons use_binreloc=0' to turn off binary relocation support
      'scons os_cc=1' to use the operating system's C compiler environment variable
      'scons os_cxx=1' to use the operating system's C++ compiler environment variable
      'scons os_cxxflags=1' to use the operating system's C++ compiler flags environment variable
      'scons use_distcc=1' to use distributed compilation
      'scons efficiency=1' to show efficiency assessment at compile time
      'scons profiling=1' to enable profiling support
%s 

Note: The options you enter will be saved in the file vdrift.conf and they will be the defaults which are used every subsequent time you run scons.""" % opts.GenerateHelpText(env))

#--------------------------#
# Check for Libs & Headers #
#--------------------------#
env.ParseConfig(env['pkg_config'] + ' bullet --libs --cflags')
conf = Configure(env)
for header in check_headers:
    if not conf.CheckCXXHeader(header):
        print('You do not have the %s headers installed. Exiting.' % header)
        Exit(1)
for lib in check_libs:
    if not conf.CheckLibWithHeader(lib[0], lib[1], 'C', lib[2]):
        print(lib[3])
        Exit(1)

env = conf.Finish()


#-------------#
# directories #
#-------------#
env['data_directory'] = env['destdir'] + env['prefix'] + '/' + env['datadir']
env['locale_directory'] = env['destdir'] + env['prefix'] + '/' + env['localedir']
cppdefines.append(("SETTINGS_DIR", '"%s"' % env['settings']))
if sys.platform in ['win32', 'msys', 'cygwin']:
    env['use_binreloc'] = False
    env['use_apbuild'] = False
    env['data_directory'] = "./data"
    env['locale_directory'] = "./data/locale"
    env['settings'] = "VDrift"
    cppdefines.append(("DATA_DIR", '"%s"' % env['data_directory']))
    cppdefines.append(("LOCALE_DIR", '"%s"' % env['locale_directory']))
#elif ('darwin' == env['PLATFORM']):
    #cppdefines.append(("DATA_DIR", "get_mac_data_dir()"))
else:
    cppdefines.append(("DATA_DIR", '"%s"' % (env['prefix'] + '/' + env['datadir'])))
    cppdefines.append(("LOCALE_DIR", '"%s"' % (env['prefix'] + '/' + env['localedir'])))

#------------------------#
# Version, debug/release #
#------------------------#
version = time.strftime("%Y-%m-%d", time.gmtime(int(os.environ.get('SOURCE_DATE_EPOCH', time.time()))))
build_dir = 'build'
if env['release']:
    # release build, debugging off, optimizations on
    env.Append(CCFLAGS = ['-O3', '-pipe'])
    build_dir = env['builddir_release']
else:
    # debug build, lots of debugging, no optimizations
    env.Append(CCFLAGS = ['-g3'])
    cppdefines.append(('DEBUG','1'))
    build_dir = env['builddir_debug']
    version = 'development'

if env['minimal']:
    version += "-minimal"
else:
    version += "-full"

#-----------#
# profiling #
#-----------#
if env['profiling']:
    env.Append(CCFLAGS = ['-pg'])
    env.Append(LINKFLAGS = ['-pg'])

#------------------------------------#
# compile-time efficiency assessment #
#------------------------------------#
if env['efficiency']:
    env.Append(CCFLAGS = ['-Weffc++'])

#-------------#
# g++ version #
#-------------#
if env['use_apbuild']:
    env['CXX'] = 'apg++'
    env['CC'] = 'apgcc'

if env['use_binreloc']:
    cppdefines.append('ENABLE_BINRELOC')

if env['use_distcc']:
    env['CXX'] = 'distcc '+env['CXX']
    env['CC'] = 'distcc '+env['CC']

#----------------------#
# OS compiler settings #
#----------------------#
if env['os_cc']:
    env.Replace(CC = os.environ['CC'])

if env['os_cxx']:
    env.Replace(CXX = os.environ['CXX'])

if env['os_cxxflags']:
    env.Append(CXXFLAGS = os.environ['CXXFLAGS'])

#------------------------------------#
# Target architecture to compile for #
#------------------------------------#
arch_flags = {
    'axp': "-march=athlon-xp",
    '686': "-march=i686",
    'p4': "-march=pentium4",
    'a64': "-march=athlon64",
    'prescott': "-march=prescott",
    'nocona': "-march=nocona",
    'core2': "-march=core2"
}
if env['arch'] in arch_flags:
    env.Append(CCFLAGS=arch_flags[env['arch']])

#---------#
# Version #
#---------#
cppdefines.append(('VERSION', '"%s"' % version))
revision = os.popen('svnversion').read().split('\n')
cppdefines.append(('REVISION', '"%s"' % revision[0]))

#---------------#
# Python Export #
#---------------#
if 'wrapper' in COMMAND_LINE_TARGETS:
  cppdefines.append("WRAPPER")

#--------------------------#
# take care of CPP defines #
#--------------------------#
write_definitions(cppdefines)

#-----------------#
# Create Archives #
#-----------------#
src_dir_name = build_dir + '/vdrift-%s-src' % version
bin_dir_name = build_dir + '/vdrift-%s-bin' % version

env.Distribute(src_dir_name, ['SConstruct'])
env.Distribute(bin_dir_name, [build_dir + '/vdrift'])
src_dir = Dir( src_dir_name )
bin_dir = Dir( bin_dir_name )

src_archive = env.Tarball('%s.tar.bz2' % src_dir_name, src_dir)
bin_archive = env.Tarball('%s.tar.bz2' % bin_dir_name, bin_dir)

#----------------#
# Target Aliases #
#----------------#
env.Alias(target = 'install', source = [Dir(env['data_directory']), Dir('$destdir$prefix/$bindir')])
env.Alias(target = 'src-package', source = src_archive)
env.Alias(target = 'bin-package', source = bin_archive)

#----------------#
# Subdirectories #
#----------------#
Export(['env', 'version', 'src_dir', 'bin_dir'])
if 'install' in COMMAND_LINE_TARGETS:
    if not os.path.isfile('data/SConscript'):
        raise 'VDrift data not found. Please make sure data is placed in vdrift directory. See README.md and http://wiki.vdrift.net.' 
    SConscript('data/SConscript')
    # desktop appdata installation
    install_desktop = env.Install(env['destdir'] + env['prefix'] + '/share/applications', 'vdrift.desktop')
    install_appdata = env.Install(env['destdir'] + env['prefix'] + '/share/metainfo', 'vdrift.appdata.xml')
    env.Alias('install', [install_desktop, install_appdata])

if 'src-package' in COMMAND_LINE_TARGETS:
    SConscript('tools/SConscript')
    SConscript('data/SConscript')

if 'data-package' in COMMAND_LINE_TARGETS:
    SConscript('data/SConscript')

if 'autopackage' in COMMAND_LINE_TARGETS:
    os.system("CXX1=g++-3.4 CXX2=g++-4.1 APBUILD_CXX1=g++-3.4 APBUILD_NO_STATIC_X=1 VDRIFT_VERSION=%s VDRIFT_MINIMAL=%d VDRIFT_RELEASE=%d makepackage tools/autopackage/vdrift.apspec" % (version, env['minimal'], env['release']))

SConscript('src/SConscript', variant_dir = build_dir, duplicate = 0)
